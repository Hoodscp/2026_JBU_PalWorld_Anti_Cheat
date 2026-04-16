import http from 'http';

const MOCK_INFO = {
  version: "0.1.5.0",
  servername: "Mock Palworld Server",
  description: "This is a mock server for the dashboard",
  worldguid: "12345678-1234-1234-1234-123456789abc"
};

const MOCK_PLAYERS = {
  players: [
    {
      name: "PlayerOne",
      accountName: "SteamUser1",
      playerId: "11112222",
      userId: "user_111",
      ip: "192.168.1.10",
      ping: 45,
      location_x: 100,
      location_y: 200,
      level: 50,
      building_count: 15
    },
    {
      name: "PlayerTwo",
      accountName: "SteamUser2",
      playerId: "33334444",
      userId: "user_222",
      ip: "192.168.1.11",
      ping: 60,
      location_x: -50,
      location_y: 300,
      level: 42,
      building_count: 5
    }
  ]
};

const MOCK_SETTINGS = {
  ServerName: "Mock Palworld Server",
  ServerDescription: "A fully mocked setting",
  ServerPlayerMaxNum: 32,
  PublicPort: 8211,
  RESTAPIPort: 8212,
  bIsPvP: false,
  Difficulty: "Normal",
  ExpRate: 2.0,
  DeathPenalty: "None",
  bEnableFastTravel: true
};

let MOCK_METRICS = {
  serverfps: 60,
  currentplayernum: 2,
  serverframetime: 16.6,
  maxplayernum: 32,
  uptime: 3600,
  days: 12,
  basecampnum: 3
};

const server = http.createServer((req, res) => {
  // CORS configuration if needed, though proxy might handle it
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  // Parse body for POST requests
  let body = '';
  req.on('data', chunk => {
    body += chunk.toString();
  });

  req.on('end', () => {
    console.log(`[Mock Server] ${req.method} ${req.url}`);
    let parsedBody = {};
    if (body) {
      try {
        parsedBody = JSON.parse(body);
        console.log(`[Mock Server] Body:`, parsedBody);
      } catch (e) { }
    }

    res.setHeader('Content-Type', 'application/json');

    // Simulate basic auth (optional, just accept anything for mock)
    // if (!req.headers.authorization) ...
    
    // Route matching
    const url = new URL(req.url, `http://${req.headers.host}`);
    const path = url.pathname;

    MOCK_METRICS.uptime += 10; // fake uptime increasing
    
    if (req.method === 'GET') {
      if (path === '/v1/api/info') {
        res.writeHead(200);
        res.end(JSON.stringify(MOCK_INFO));
      } else if (path === '/v1/api/players') {
        res.writeHead(200);
        res.end(JSON.stringify(MOCK_PLAYERS));
      } else if (path === '/v1/api/settings') {
        res.writeHead(200);
        res.end(JSON.stringify(MOCK_SETTINGS));
      } else if (path === '/v1/api/metrics') {
        res.writeHead(200);
        res.end(JSON.stringify(MOCK_METRICS));
      } else {
        res.writeHead(404);
        res.end(JSON.stringify({ error: 'Not Found' }));
      }
    } else if (req.method === 'POST') {
      if (path === '/v1/api/save') {
        res.writeHead(200);
        res.end(JSON.stringify({ success: true }));
      } else if (path === '/v1/api/announce') {
        res.writeHead(200);
        res.end(JSON.stringify({ success: true, message: parsedBody.message }));
      } else if (path === '/v1/api/shutdown' || path === '/v1/api/forceshutdown') {
        res.writeHead(200);
        res.end(JSON.stringify({ success: true }));
      } else if (path === '/v1/api/kick' || path === '/v1/api/ban' || path === '/v1/api/unban') {
        res.writeHead(200);
        res.end(JSON.stringify({ success: true }));
      } else {
        res.writeHead(404);
        res.end(JSON.stringify({ error: 'Not Found' }));
      }
    } else {
      res.writeHead(405);
      res.end();
    }
  });
});

const PORT = 8212;
server.listen(PORT, () => {
  console.log(`Mock server is running on http://localhost:${PORT}`);
});
