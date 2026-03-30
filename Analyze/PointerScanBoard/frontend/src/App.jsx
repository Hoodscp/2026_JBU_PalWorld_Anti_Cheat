import { useState, useCallback } from 'react'

function TreeNode({ node }) {
  const [expanded, setExpanded] = useState(true);
  const targets = node.targets || [];
  const children = node.children || [];
  
  return (
    <div className="tree-node">
      <div className="node-content" onClick={() => setExpanded(!expanded)} style={{ cursor: children.length ? 'pointer' : 'default' }}>
        {children.length > 0 && (
          <span style={{ fontSize: '0.8rem', color: '#94a3b8', marginRight: '4px' }}>
            {expanded ? '▼' : '▶'}
          </span>
        )}
        <span className="node-value">{node.value}</span>
        {targets.length > 0 && (
          <div className="node-targets">
            {targets.map((t, idx) => (
              <span key={idx} className="target-badge">T: {t}</span>
            ))}
          </div>
        )}
      </div>
      {expanded && children.length > 0 && (
        <div className="children-list">
          {children.map((child, idx) => (
            <TreeNode key={`${child.value}-${idx}`} node={child} />
          ))}
        </div>
      )}
    </div>
  );
}

function App() {
  const [files, setFiles] = useState([]);
  const [isDragging, setIsDragging] = useState(false);
  const [trees, setTrees] = useState(null);
  const [totalValid, setTotalValid] = useState(null);
  const [uniqueValid, setUniqueValid] = useState(null);
  const [loading, setLoading] = useState(false);
  const [exporting, setExporting] = useState(false);
  const [error, setError] = useState(null);
  
  // Basic
  const [intersectionOnly, setIntersectionOnly] = useState(false);
  
  // Advanced Settings
  const [showAdvanced, setShowAdvanced] = useState(false);
  const [targetModule, setTargetModule] = useState("");
  const [maxDepth, setMaxDepth] = useState("7");
  const [maxOffset, setMaxOffset] = useState("");
  const [align8, setAlign8] = useState(false);

  const handleDrag = useCallback((e) => {
    e.preventDefault();
    e.stopPropagation();
    if (e.type === "dragenter" || e.type === "dragover") {
      setIsDragging(true);
    } else if (e.type === "dragleave") {
      setIsDragging(false);
    }
  }, []);

  const handleDrop = useCallback((e) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(false);
    
    if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      const newFiles = Array.from(e.dataTransfer.files).filter(f => f.name.endsWith('.sqlite'));
      setFiles(prev => [...prev, ...newFiles]);
    }
  }, []);

  const handleFileInput = (e) => {
    if (e.target.files && e.target.files.length > 0) {
      const newFiles = Array.from(e.target.files);
      setFiles(prev => [...prev, ...newFiles]);
    }
  };

  const removeFile = (index) => {
    setFiles(files.filter((_, i) => i !== index));
    setTrees(null);
    setTotalValid(null);
    setUniqueValid(null);
  };

  const appendParams = (formData) => {
    formData.append('intersection_only', intersectionOnly.toString());
    formData.append('target_module', targetModule);
    formData.append('align_8_bytes', align8.toString());
    formData.append('max_offset_val', maxOffset);
    formData.append('max_depth', maxDepth);
  }

  const analyzeFiles = async () => {
    if (files.length === 0) return;
    
    setLoading(true);
    setError(null);
    
    const formData = new FormData();
    files.forEach(file => {
      formData.append('files', file);
    });
    appendParams(formData);

    try {
      const response = await fetch('http://localhost:8000/api/analyze', {
        method: 'POST',
        body: formData,
      });
      
      if (!response.ok) throw new Error('Analysis failed');
      
      const data = await response.json();
      setTrees(data.trees);
      setTotalValid(data.total_valid);
      setUniqueValid(data.unique_valid);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  const exportFiltered = async () => {
    if (files.length === 0) return;
    
    setExporting(true);
    setError(null);
    
    const formData = new FormData();
    files.forEach(file => {
      formData.append('files', file);
    });
    // The export API applies intersection and filter heuristics before deletion.
    formData.append('intersection_only', "true"); // Always true for export as user wants the filtered common
    formData.append('target_module', targetModule);
    formData.append('align_8_bytes', align8.toString());
    formData.append('max_offset_val', maxOffset);
    formData.append('max_depth', maxDepth);

    try {
      const response = await fetch('http://localhost:8000/api/export', {
        method: 'POST',
        body: formData,
      });
      
      if (!response.ok) throw new Error('Export failed. Check backend logs.');
      
      const blob = await response.blob();
      const url = window.URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'filtered_pointers.zip';
      document.body.appendChild(a);
      a.click();
      a.remove();
      window.URL.revokeObjectURL(url);
    } catch (err) {
      setError(err.message);
    } finally {
      setExporting(false);
    }
  };

  return (
    <div className="app-container">
      <header>
        <h1>Pointer Analyzer</h1>
        <p>Advanced Game Hacking Structural Pattern Detection</p>
      </header>

      <div className="glass-panel">
        <div 
          className={`upload-area ${isDragging ? 'drag-over' : ''}`}
          onDragEnter={handleDrag}
          onDragLeave={handleDrag}
          onDragOver={handleDrag}
          onDrop={handleDrop}
          onClick={() => document.getElementById('fileUpload').click()}
        >
          <span className="upload-icon">📁</span>
          <h3>Drag & Drop .sqlite exports here</h3>
          <p style={{ color: 'var(--text-secondary)' }}>or click to select files</p>
          <input 
            type="file" 
            id="fileUpload" 
            multiple 
            accept=".sqlite" 
            style={{ display: 'none' }} 
            onChange={handleFileInput}
          />
        </div>

        {files.length > 0 && (
          <div style={{ textAlign: 'center', marginTop: '1.5rem' }}>
            <div className="file-list">
              {files.map((f, i) => (
                <div key={i} className="file-tag">
                  {f.name}
                  <button onClick={(e) => { e.stopPropagation(); removeFile(i); }}>×</button>
                </div>
              ))}
            </div>

            <div style={{ margin: '1.5rem 0 1rem', display: 'flex', justifyContent: 'center', alignItems: 'center', gap: '0.75rem' }}>
              <input 
                type="checkbox" 
                id="intersectCheck" 
                checked={intersectionOnly}
                onChange={(e) => setIntersectionOnly(e.target.checked)}
                style={{ width: '1.2rem', height: '1.2rem', cursor: 'pointer' }}
              />
              <label htmlFor="intersectCheck" style={{ cursor: 'pointer', userSelect: 'none', color: 'var(--text-primary)', fontWeight: 500 }}>
                Strict Intersection (모든 파일에 공통인 체인만)
              </label>
              <button 
                className="btn-secondary" 
                style={{ marginLeft: '1rem' }}
                onClick={() => setShowAdvanced(!showAdvanced)}
              >
                {showAdvanced ? 'Hide Advanced Filters ⚙️' : 'Show Advanced Filters ⚙️'}
              </button>
            </div>

            {showAdvanced && (
              <div className="settings-panel">
                <h3 style={{ margin: '0 0 0.5rem 0', color: 'var(--accent)' }}>Heuristic Filters</h3>
                <p style={{ margin: '0 0 1rem 0', fontSize: '0.85rem', color: 'var(--text-secondary)' }}>
                  Refine pointer quality based on game engine characteristics. Only valid chains are parsed.
                </p>
                <div className="settings-grid">
                  <div className="setting-item">
                    <label>Base Module Filter (e.g. Palworld-Win64-Shipping.exe)</label>
                    <input 
                      type="text" 
                      placeholder="Leave empty for all modules" 
                      value={targetModule}
                      onChange={(e) => setTargetModule(e.target.value)}
                    />
                  </div>
                  <div className="setting-item">
                    <label>Max Depth Level (offsetcount)</label>
                    <input 
                      type="number" 
                      placeholder="7" 
                      value={maxDepth}
                      onChange={(e) => setMaxDepth(e.target.value)}
                    />
                  </div>
                  <div className="setting-item">
                    <label>Max Offset Hex Value (e.g. 0x3000)</label>
                    <input 
                      type="text" 
                      placeholder="Leave empty for unlimited" 
                      value={maxOffset}
                      onChange={(e) => setMaxOffset(e.target.value)}
                    />
                  </div>
                  <div className="setting-item" style={{ flexDirection: 'row', alignItems: 'center', gap: '1rem', paddingTop: '1.5rem' }}>
                    <input 
                      type="checkbox" 
                      id="align8Check"
                      checked={align8}
                      onChange={(e) => setAlign8(e.target.checked)}
                      style={{ width: '1.2rem', height: '1.2rem', cursor: 'pointer' }}
                    />
                    <label htmlFor="align8Check" style={{ cursor: 'pointer', color: 'var(--text-primary)' }}>
                      Require 8-byte Alignment (UE5 Optimization)
                    </label>
                  </div>
                </div>
              </div>
            )}
            
            <div style={{ display: 'flex', justifyContent: 'center', gap: '1rem', marginTop: '1.5rem' }}>
              <button 
                className="btn" 
                onClick={analyzeFiles} 
                disabled={loading || exporting}
              >
                {loading ? 'Analyzing...' : `Analyze Tree`}
              </button>

              <button 
                className="btn" 
                style={{ background: 'var(--success)' }}
                onClick={exportFiltered} 
                disabled={exporting || loading}
              >
                {exporting ? 'Exporting...' : `Export Filtered to .zip`}
              </button>
            </div>
          </div>
        )}
      </div>

      {error && (
        <div className="glass-panel" style={{ borderLeft: '4px solid var(--danger)' }}>
          <h3 style={{ color: 'var(--danger)', margin: '0 0 0.5rem 0' }}>Error</h3>
          <p style={{ margin: 0 }}>{error}</p>
        </div>
      )}

      {trees && (
        <div className="glass-panel">
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '1rem' }}>
            <h2 style={{ margin: 0 }}>Analysis Results</h2>
            {totalValid !== null && (
              <span className="file-tag" style={{ border: 'none', background: 'var(--accent)', color: '#fff', fontSize: '0.9rem', padding: '0.4rem 1rem' }}>
                Found {uniqueValid} unique chains ({totalValid} total elements)
              </span>
            )}
          </div>
          <div className="tree-container">
            {trees.length > 0 ? (
              trees.map((rootNode, idx) => (
                <TreeNode key={idx} node={rootNode} />
              ))
            ) : (
              <p style={{ color: 'var(--text-secondary)' }}>No common pointer chains found that match the criteria.</p>
            )}
          </div>
        </div>
      )}
    </div>
  )
}

export default App
