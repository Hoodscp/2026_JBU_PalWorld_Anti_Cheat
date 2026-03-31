from fastapi import FastAPI, UploadFile, File, Form, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from starlette.background import BackgroundTask
import sqlite3
import shutil
import os
import zipfile
from tempfile import NamedTemporaryFile, mkdtemp

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
def read_root():
    return {"status": "Analyzer Backend is running."}

def insert_into_tree(tree, base, chain, target_offset, file_name):
    if base not in tree:
        tree[base] = {"value": base, "children": {}, "targets": set()}
    
    current = tree[base]
    for offset in chain:
        off_hex = hex(offset)
        if off_hex not in current["children"]:
            current["children"][off_hex] = {"value": off_hex, "children": {}, "targets": set()}
        current = current["children"][off_hex]
    
    current["targets"].add((hex(target_offset), file_name))

def format_tree(node):
    return {
        "value": node["value"],
        "targets": [{"offset": t[0], "file": t[1]} for t in list(node["targets"])],
        "children": [format_tree(c) for c in node["children"].values()]
    }

def parse_hex_or_int(val_str):
    try:
        val_str = str(val_str).strip()
        if not val_str:
            return None
        return int(val_str, 16) if val_str.lower().startswith("0x") else int(val_str)
    except:
        return None

def process_files(files, intersection_mode="strict", min_depth=2, target_module="", align_8_bytes=False, max_offset_val="", max_depth_val="7"):
    file_signatures = [] 
    file_data = [] 
    
    max_off = parse_hex_or_int(max_offset_val)
    max_d = parse_hex_or_int(max_depth_val)

    for file in files:
        temp_file = NamedTemporaryFile(delete=False, suffix=".sqlite")
        shutil.copyfileobj(file.file, temp_file)
        temp_file.close()
        
        con = sqlite3.connect(temp_file.name)
        con.row_factory = sqlite3.Row
        cur = con.cursor()
        
        try:
            cur.execute("SELECT moduleid, name FROM modules")
            modules = {row["moduleid"]: row["name"] for row in cur.fetchall()}
        except Exception:
            modules = {}
            
        try:
            cur.execute("SELECT ptrid, offsetvalue FROM pointerfiles_endwithoffsetlist")
            end_offsets = {row["ptrid"]: row["offsetvalue"] for row in cur.fetchall()}
        except Exception:
            end_offsets = {}
            
        cur.execute("SELECT * FROM results")
        results = cur.fetchall()
        
        signatures = set()
        file_results = []
        
        for row in results:
            ptrid = row["ptrid"]
            resultid = row["resultid"]
            offset_count = row["offsetcount"]
            module_id = row["moduleid"]
            module_offset = row["moduleoffset"]
            
            if max_d is not None and offset_count > max_d:
                continue

            module_name = modules.get(module_id, "UnknownModule")
            if target_module and target_module.lower() not in module_name.lower():
                continue
            
            chain = []
            valid_chain = True
            # CE SQLite stores offset1 as the last offset applied (leaf), and offset<N> as the first offset applied (base).
            # We want the chain from base to leaf, so we iterate from offset_count down to 1.
            for i in range(offset_count, 0, -1):
                col_name = f"offset{i}"
                if col_name in row.keys() and row[col_name] is not None:
                    off_val = row[col_name]
                    
                    if align_8_bytes and (off_val % 8 != 0):
                        valid_chain = False
                        break
                        
                    if max_off is not None and off_val > max_off:
                        valid_chain = False
                        break
                        
                    chain.append(off_val)
            
            if not valid_chain:
                continue
                
            base_str = f"{module_name}+{hex(module_offset)}"
            target_offset = end_offsets.get(ptrid, 0)
            
            sig = (base_str, tuple(chain))
            signatures.add(sig)
            
            file_results.append({
                "resultid": resultid,
                "sig": sig,
                "base_str": base_str,
                "chain": chain,
                "target_offset": target_offset
            })
            
        con.close()
        file_signatures.append(signatures)
        file_data.append({
            "path": temp_file.name,
            "filename": file.filename,
            "results": file_results
        })

    valid_ids_per_file = []
    combined_tree = {}
    unique_valid_set = set()
    total_files = len(files)

    valid_signatures = set()
    prefix_map = {}

    if intersection_mode == "strict":
        valid_signatures = set.intersection(*file_signatures) if file_signatures else set()
    elif intersection_mode == "union":
        valid_signatures = set.union(*file_signatures) if file_signatures else set()
    elif intersection_mode == "partial":
        for idx, sigs in enumerate(file_signatures):
            for sig in sigs:
                base, chain = sig
                for d in range(min_depth, len(chain) + 1):
                    prefix = (base, tuple(chain[:d]))
                    if prefix not in prefix_map:
                        prefix_map[prefix] = set()
                    prefix_map[prefix].add(idx)
    
    for idx, fd in enumerate(file_data):
        valid_ids = set()
        filename = os.path.splitext(os.path.basename(fd["filename"]))[0]
        
        for r in fd["results"]:
            sig = r["sig"]
            base, chain = sig
            is_valid = False
            
            if intersection_mode in ["strict", "union"]:
                if sig in valid_signatures:
                    is_valid = True
            elif intersection_mode == "partial":
                for d in range(min_depth, len(chain) + 1):
                    prefix = (base, tuple(chain[:d]))
                    if len(prefix_map.get(prefix, set())) == total_files:
                        is_valid = True
                        break
            
            if is_valid:
                valid_ids.add(r["resultid"])
                unique_valid_set.add(sig)
                insert_into_tree(combined_tree, r["base_str"], r["chain"], r["target_offset"], filename)
                
        valid_ids_per_file.append(valid_ids)
        
    return combined_tree, valid_ids_per_file, file_data, len(unique_valid_set)

@app.post("/api/analyze")
async def analyze_sqlite(
    intersection_mode: str = Form("strict"), 
    min_shared_depth: int = Form(2),
    target_module: str = Form(""),
    align_8_bytes: str = Form("false"),
    max_offset_val: str = Form(""),
    max_depth: str = Form("7"),
    files: list[UploadFile] = File(...)):
    
    is_align = align_8_bytes.lower() == "true"
    
    try:
        combined_tree, valid_ids_per_file, file_data, unique_valid = process_files(
            files, intersection_mode, min_shared_depth, target_module, is_align, max_offset_val, max_depth
        )
        
        for fd in file_data:
            if os.path.exists(fd["path"]):
                os.unlink(fd["path"])
                
        total_valid = sum(len(ids) for ids in valid_ids_per_file)
        formatted_roots = [format_tree(node) for node in combined_tree.values()]
        return {"status": "success", "trees": formatted_roots, "total_valid": total_valid, "unique_valid": unique_valid}
    except Exception as e:
        import traceback; traceback.print_exc()
        raise HTTPException(status_code=500, detail=str(e))

def cleanup_temp_dir(temp_dir, file_data):
    for fd in file_data:
        if os.path.exists(fd["path"]):
            try:
                os.unlink(fd["path"])
            except:
                pass
    try:
        shutil.rmtree(temp_dir)
    except:
        pass

@app.post("/api/export")
async def export_sqlite(
    intersection_mode: str = Form("strict"), 
    min_shared_depth: int = Form(2),
    target_module: str = Form(""),
    align_8_bytes: str = Form("false"),
    max_offset_val: str = Form(""),
    max_depth: str = Form("7"),
    files: list[UploadFile] = File(...)):
    
    is_align = align_8_bytes.lower() == "true"

    try:
        _, valid_ids_per_file, file_data, _ = process_files(
            files, intersection_mode, min_shared_depth, target_module, is_align, max_offset_val, max_depth
        )
        
        temp_dir = mkdtemp()
        zip_path = os.path.join(temp_dir, "filtered_pointers.zip")
        
        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for idx, fd in enumerate(file_data):
                con = sqlite3.connect(fd["path"])
                cur = con.cursor()
                
                valid_ids_list = list(valid_ids_per_file[idx])
                
                cur.execute("CREATE TEMP TABLE keep_ids (id INTEGER PRIMARY KEY)")
                chunk_size = 900
                for i in range(0, len(valid_ids_list), chunk_size):
                    chunk = [(vid,) for vid in valid_ids_list[i:i+chunk_size]]
                    cur.executemany("INSERT INTO keep_ids (id) VALUES (?)", chunk)
                
                cur.execute("DELETE FROM results WHERE resultid NOT IN (SELECT id FROM keep_ids)")
                
                con.commit()
                con.execute("VACUUM")
                con.commit()
                
                con.close()
                
                base_name = os.path.basename(fd["filename"]) 
                name, ext = os.path.splitext(base_name)
                final_name = f"{name}_filtered{ext}"
                zipf.write(fd["path"], arcname=final_name)

        return FileResponse(
            zip_path, 
            media_type="application/zip", 
            filename="filtered_pointers.zip",
            background=BackgroundTask(cleanup_temp_dir, temp_dir, file_data)
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
