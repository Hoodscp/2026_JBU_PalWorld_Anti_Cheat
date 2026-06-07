// DashboardServer.h  (External)
// 탐지 이벤트를 웹 페이지로 보여주기 위한 초경량 임베디드 HTTP 서버(헤더 전용).
//
// 설계 의도:
//   - 콘솔 로그(기존)는 그대로 두고, 같은 DetectionEvent 를 이 서버에도 Push 한다.
//   - 외부 라이브러리 없이 Winsock 만 사용. localhost 대시보드 용도라 단순/동기 처리.
//   - GET  /                     → 단일 페이지 대시보드(HTML, 모듈별 탭).
//   - GET  /api/events?since=N   → id>N 인 이벤트 JSON 배열(증분 폴링).
//   - 이벤트는 링버퍼(최근 N개)로 보관. 클라이언트는 마지막 id 이후만 받아 append.
//
// 보안 메모: 인증 없음. 기본 바인드 주소는 127.0.0.1(루프백) 권장.
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include "IDetectionModule.h"  // ac::DetectionEvent / Severity

#include <atomic>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

namespace ac {

inline const char* DashSevName(Severity s) {
    switch (s) {
        case Severity::Info:     return "INFO";
        case Severity::Low:      return "LOW";
        case Severity::Medium:   return "MEDIUM";
        case Severity::High:     return "HIGH";
        case Severity::Critical: return "CRITICAL";
    }
    return "INFO";
}

class DashboardServer {
public:
    DashboardServer() = default;
    ~DashboardServer() { Stop(); }

    DashboardServer(const DashboardServer&) = delete;
    DashboardServer& operator=(const DashboardServer&) = delete;

    // 127.0.0.1:port 에서 수신 시작. 실패 시 false(콘솔 로그는 영향 없음).
    bool Start(uint16_t port) {
        port_ = port;
        WSADATA wsa{};
        if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
        wsa_ = true;

        listen_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_ == INVALID_SOCKET) { Cleanup(); return false; }

        BOOL yes = TRUE;
        ::setsockopt(listen_, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const char*>(&yes), sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = ::htons(port);
        ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);  // 루프백만

        if (::bind(listen_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            Cleanup(); return false;
        }
        if (::listen(listen_, SOMAXCONN) == SOCKET_ERROR) { Cleanup(); return false; }

        running_.store(true);
        th_ = std::thread(&DashboardServer::AcceptLoop, this);
        return true;
    }

    void Stop() {
        bool was = running_.exchange(false);
        if (listen_ != INVALID_SOCKET) {
            ::closesocket(listen_);      // accept() 깨우기
            listen_ = INVALID_SOCKET;
        }
        if (th_.joinable()) th_.join();
        Cleanup();
        if (!was) return;
    }

    uint16_t Port() const { return port_; }

    // 탐지 이벤트 추가(콘솔 출력과 별개로 호출). id 부여 후 링버퍼에 보관.
    void Push(const DetectionEvent& ev) {
        std::lock_guard<std::mutex> lock(mtx_);
        items_.push_back(Item{ nextId_++, ev });
        while (items_.size() > maxItems_) items_.pop_front();
    }

private:
    struct Item { uint64_t id; DetectionEvent ev; };

    void Cleanup() {
        if (listen_ != INVALID_SOCKET) { ::closesocket(listen_); listen_ = INVALID_SOCKET; }
        if (wsa_) { ::WSACleanup(); wsa_ = false; }
    }

    void AcceptLoop() {
        while (running_.load()) {
            sockaddr_in cli{};
            int len = sizeof(cli);
            SOCKET c = ::accept(listen_, reinterpret_cast<sockaddr*>(&cli), &len);
            if (c == INVALID_SOCKET) {
                if (!running_.load()) break;  // Stop() 으로 닫힘
                continue;
            }
            HandleClient(c);
            ::closesocket(c);
        }
    }

    void HandleClient(SOCKET c) {
        // 요청 첫 줄만 필요. 한 번 recv 로 충분(작은 GET 요청).
        char buf[4096];
        int n = ::recv(c, buf, sizeof(buf) - 1, 0);
        if (n <= 0) return;
        buf[n] = '\0';

        // "GET <path> HTTP/1.1" 파싱
        std::string req(buf, static_cast<size_t>(n));
        std::string path = "/";
        {
            size_t sp1 = req.find(' ');
            if (sp1 != std::string::npos) {
                size_t sp2 = req.find(' ', sp1 + 1);
                if (sp2 != std::string::npos) path = req.substr(sp1 + 1, sp2 - sp1 - 1);
            }
        }

        if (path.rfind("/api/events", 0) == 0) {
            uint64_t since = 0;
            size_t q = path.find("since=");
            if (q != std::string::npos) {
                since = ::_strtoui64(path.c_str() + q + 6, nullptr, 10);
            }
            std::string body = BuildEventsJson(since);
            SendResponse(c, "200 OK", "application/json; charset=utf-8", body);
        } else if (path == "/" || path.rfind("/index.html", 0) == 0) {
            SendResponse(c, "200 OK", "text/html; charset=utf-8", IndexHtml());
        } else {
            SendResponse(c, "404 Not Found", "text/plain; charset=utf-8", "not found");
        }
    }

    static void SendResponse(SOCKET c, const char* status, const char* ctype,
                             const std::string& body) {
        char header[256];
        int hn = std::snprintf(header, sizeof(header),
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Cache-Control: no-store\r\n"
            "Connection: close\r\n"
            "\r\n",
            status, ctype, body.size());
        if (hn > 0) ::send(c, header, hn, 0);
        // 큰 본문도 안전하게 전송.
        size_t sent = 0;
        while (sent < body.size()) {
            int chunk = static_cast<int>(
                (body.size() - sent > 32768) ? 32768 : (body.size() - sent));
            int w = ::send(c, body.data() + sent, chunk, 0);
            if (w <= 0) break;
            sent += static_cast<size_t>(w);
        }
    }

    static std::string JsonEscape(const std::string& s) {
        std::string o;
        o.reserve(s.size() + 8);
        for (unsigned char ch : s) {
            switch (ch) {
                case '"':  o += "\\\""; break;
                case '\\': o += "\\\\"; break;
                case '\n': o += "\\n";  break;
                case '\r': o += "\\r";  break;
                case '\t': o += "\\t";  break;
                default:
                    if (ch < 0x20) {  // 기타 제어문자
                        char u[8];
                        std::snprintf(u, sizeof(u), "\\u%04x", ch);
                        o += u;
                    } else {
                        o += static_cast<char>(ch);  // UTF-8 바이트 그대로
                    }
            }
        }
        return o;
    }

    std::string BuildEventsJson(uint64_t since) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::string out = "{\"lastId\":" + std::to_string(nextId_ - 1) + ",\"events\":[";
        bool first = true;
        for (const auto& it : items_) {
            if (it.id <= since) continue;
            if (!first) out += ',';
            first = false;
            out += "{\"id\":" + std::to_string(it.id);
            out += ",\"ts\":" + std::to_string(it.ev.timestampMs);
            out += ",\"module\":\"" + JsonEscape(it.ev.moduleName) + "\"";
            out += ",\"type\":\""   + JsonEscape(it.ev.detectionType) + "\"";
            out += ",\"sev\":\""    + std::string(DashSevName(it.ev.severity)) + "\"";
            out += ",\"desc\":\""   + JsonEscape(it.ev.description) + "\"";
            out += ",\"evidence\":\"" + JsonEscape(it.ev.evidence) + "\"";
            out += '}';
        }
        out += "]}";
        return out;
    }

    static const std::string& IndexHtml();

    std::mutex          mtx_;
    std::deque<Item>    items_;
    uint64_t            nextId_   = 1;
    size_t              maxItems_ = 5000;

    SOCKET            listen_ = INVALID_SOCKET;
    std::thread       th_;
    std::atomic<bool> running_{ false };
    bool              wsa_  = false;
    uint16_t          port_ = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// 대시보드 단일 페이지. 의존성 없는 vanilla JS/CSS, 다크 테마, 모듈별 탭.
// /api/events?since=N 을 1초마다 폴링해 증분 append.
// ─────────────────────────────────────────────────────────────────────────────
inline const std::string& DashboardServer::IndexHtml() {
    static const std::string html = R"HTML(<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>JBU Palworld Anti-Cheat — Detection Dashboard</title>
<style>
  :root{
    --bg:#0d1117; --panel:#161b22; --border:#30363d; --text:#e6edf3;
    --muted:#8b949e; --accent:#58a6ff;
    --info:#6e7681; --low:#3fb950; --medium:#d29922; --high:#f85149; --critical:#ff5c8a;
  }
  *{box-sizing:border-box}
  body{margin:0;font-family:Segoe UI,Malgun Gothic,system-ui,sans-serif;
       background:var(--bg);color:var(--text);font-size:14px}
  header{padding:14px 20px;border-bottom:1px solid var(--border);
         display:flex;align-items:center;gap:14px;flex-wrap:wrap;
         position:sticky;top:0;background:var(--bg);z-index:5}
  header h1{font-size:16px;margin:0;font-weight:600}
  .status{display:flex;align-items:center;gap:6px;color:var(--muted);font-size:12px}
  .dot{width:8px;height:8px;border-radius:50%;background:var(--high)}
  .dot.on{background:var(--low)}
  .spacer{flex:1}
  .controls{display:flex;gap:10px;align-items:center;color:var(--muted);font-size:12px}
  .controls input[type=text]{background:var(--panel);border:1px solid var(--border);
     color:var(--text);border-radius:6px;padding:5px 8px;width:180px}
  button.btn{background:var(--panel);border:1px solid var(--border);color:var(--text);
     border-radius:6px;padding:5px 10px;cursor:pointer}
  button.btn:hover{border-color:var(--accent)}
  nav{display:flex;gap:6px;padding:12px 20px;flex-wrap:wrap;
      border-bottom:1px solid var(--border)}
  .tab{padding:6px 12px;border:1px solid var(--border);border-radius:20px;
       cursor:pointer;color:var(--muted);background:var(--panel);
       display:flex;align-items:center;gap:8px;user-select:none}
  .tab:hover{border-color:var(--accent)}
  .tab.active{color:var(--text);border-color:var(--accent);background:#1f6feb22}
  .tab .cnt{background:#30363d;border-radius:10px;padding:0 7px;font-size:11px;color:var(--text)}
  .tab .cnt.crit{background:var(--critical);color:#000}
  main{padding:16px 20px}
  table{width:100%;border-collapse:collapse}
  th,td{text-align:left;padding:8px 10px;border-bottom:1px solid var(--border);
        vertical-align:top}
  th{color:var(--muted);font-weight:600;font-size:12px;position:sticky;top:104px;
     background:var(--bg)}
  td.ev{font-family:Consolas,monospace;font-size:12px;color:var(--muted);
        word-break:break-all;max-width:680px}
  td.time{color:var(--muted);white-space:nowrap;font-variant-numeric:tabular-nums}
  td.mod{white-space:nowrap;font-weight:600}
  .pill{display:inline-block;padding:2px 9px;border-radius:12px;font-size:11px;
        font-weight:700;color:#0d1117}
  .sev-INFO{background:var(--info);color:#fff}
  .sev-LOW{background:var(--low)}
  .sev-MEDIUM{background:var(--medium)}
  .sev-HIGH{background:var(--high);color:#fff}
  .sev-CRITICAL{background:var(--critical)}
  tr.new{animation:flash 1.2s ease-out}
  @keyframes flash{from{background:#1f6feb33}to{background:transparent}}
  .empty{padding:48px;text-align:center;color:var(--muted)}
  .modtag{font-size:11px;color:var(--muted)}
</style>
</head>
<body>
<header>
  <h1>🛡️ JBU Palworld Anti-Cheat — Detection Dashboard</h1>
  <div class="status"><span id="dot" class="dot"></span><span id="statusText">연결 중…</span></div>
  <div class="spacer"></div>
  <div class="controls">
    <input id="filter" type="text" placeholder="검색(type/evidence)…">
    <label><input type="checkbox" id="autoscroll" checked> 자동 최신</label>
    <button class="btn" id="clearBtn">화면 비우기</button>
  </div>
</header>
<nav id="tabs"></nav>
<main>
  <table>
    <thead>
      <tr><th style="width:96px">시각</th><th style="width:80px">심각도</th>
          <th style="width:150px">모듈</th><th style="width:210px">유형</th>
          <th>근거(evidence)</th></tr>
    </thead>
    <tbody id="rows"></tbody>
  </table>
  <div id="empty" class="empty">아직 탐지 이벤트가 없습니다. (대상 프로세스 감시 중)</div>
</main>
<script>
// 알려진 모듈 라벨(없으면 모듈명 그대로). 사용자가 요청한 "따로 보기" 탭 구성.
const MOD_LABEL = {
  "AntiDebug":"안티디버깅", "InjectionScanner":"인젝션", "HookScanner":"후킹 감지",
  "ProcessMonitor":"프로세스 감시", "IntegrityScanner":"무결성"
};
const SEV_RANK = {INFO:0,LOW:1,MEDIUM:2,HIGH:3,CRITICAL:4};
let events = [];        // 전체 이벤트
let lastId = 0;
let active = "ALL";     // 현재 탭(모듈명 또는 ALL)
let filterText = "";

const $ = s => document.querySelector(s);
function fmtTime(ms){ const d=new Date(Number(ms));
  return d.toLocaleTimeString('ko-KR',{hour12:false}); }
function modLabel(m){ return MOD_LABEL[m]||m; }

function moduleList(){
  const set = new Set(events.map(e=>e.module));
  // 알려진 모듈을 먼저, 나머지는 등장 순.
  const known = Object.keys(MOD_LABEL).filter(m=>set.has(m));
  const extra = [...set].filter(m=>!MOD_LABEL[m]);
  return known.concat(extra);
}

function renderTabs(){
  const mods = moduleList();
  const tabs = $("#tabs");
  const counts = {ALL:events.length}, crit={ALL:0};
  for(const e of events){ counts[e.module]=(counts[e.module]||0)+1;
    if(e.sev==="CRITICAL"){crit[e.module]=(crit[e.module]||0)+1; crit.ALL++;} }
  const make=(key,label)=>{
    const c=counts[key]||0, k=crit[key]||0;
    return `<div class="tab ${active===key?'active':''}" data-k="${key}">`+
      `${label}<span class="cnt ${k>0?'crit':''}">${c}</span></div>`;
  };
  let html = make("ALL","전체");
  for(const m of mods) html += make(m, modLabel(m));
  tabs.innerHTML = html;
  tabs.querySelectorAll('.tab').forEach(t=>t.onclick=()=>{
    active=t.dataset.k; renderTabs(); renderRows();
  });
}

function visible(){
  let v = active==="ALL" ? events : events.filter(e=>e.module===active);
  if(filterText){
    const f=filterText.toLowerCase();
    v = v.filter(e=>(e.type+' '+e.evidence+' '+e.desc).toLowerCase().includes(f));
  }
  return v;
}

function rowHtml(e,isNew){
  return `<tr class="${isNew?'new':''}">`+
    `<td class="time">${fmtTime(e.ts)}</td>`+
    `<td><span class="pill sev-${e.sev}">${e.sev}</span></td>`+
    `<td class="mod">${modLabel(e.module)}<div class="modtag">${e.module}</div></td>`+
    `<td>${escapeHtml(e.type)}<div class="modtag">${escapeHtml(e.desc)}</div></td>`+
    `<td class="ev">${escapeHtml(e.evidence)}</td></tr>`;
}
function escapeHtml(s){ return (s||'').replace(/[&<>"]/g,
  c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c])); }

let renderedIds = new Set();
function renderRows(){
  const v = visible();
  $("#empty").style.display = v.length? 'none':'block';
  const rows = $("#rows");
  // 최신이 위로.
  const ordered = v.slice().reverse();
  rows.innerHTML = ordered.map(e=>rowHtml(e, !renderedIds.has(e.id))).join('');
  ordered.forEach(e=>renderedIds.add(e.id));
  if($("#autoscroll").checked) window.scrollTo({top:0});
}

async function poll(){
  try{
    const r = await fetch('/api/events?since='+lastId, {cache:'no-store'});
    const j = await r.json();
    if(j.events && j.events.length){
      events = events.concat(j.events);
      if(events.length>8000) events = events.slice(events.length-8000);
      lastId = j.lastId;
      renderTabs(); renderRows();
    }
    setStatus(true);
  }catch(e){ setStatus(false); }
}
function setStatus(ok){
  $("#dot").className = "dot"+(ok?" on":"");
  $("#statusText").textContent = ok ? "실시간 감시 중" : "서버 연결 끊김";
}

$("#filter").addEventListener('input', e=>{ filterText=e.target.value; renderRows(); });
$("#clearBtn").addEventListener('click', ()=>{ events=[]; renderedIds.clear();
  renderTabs(); renderRows(); });

renderTabs(); renderRows();
poll();
setInterval(poll, 1000);
</script>
</body>
</html>)HTML";
    return html;
}

}  // namespace ac
