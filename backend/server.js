const express = require('express');
const { spawn } = require('child_process');
const bodyParser = require('body-parser');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 5000;

app.use(cors());
app.use(bodyParser.json());

// Spawn the C process
// Ensure dsa2.exe exists in the parent directory or same directory. 
// Adjust path as needed.
const dsaPath = path.join(__dirname, '../dsa2'); // Assumes compiled dsa2.exe is in project root
console.log(`Spawning C process at: ${dsaPath}`);

const dsaProcess = spawn(dsaPath, ['--api']);

dsaProcess.on('error', (err) => {
    console.error('Failed to start C process:', err);
    console.log('HINT: Did you forget to compile? Run: gcc dsa2.c -o dsa2');
});

dsaProcess.on('close', (code) => {
    console.log(`C process exited with code ${code}`);
});

// Buffer to store data from C stdout until a newline is found
let dataBuffer = '';
let responseQueue = []; // Queue of callbacks waiting for responses

dsaProcess.stdout.on('data', (data) => {
    dataBuffer += data.toString();

    // Process all complete lines
    let newlineIdx;
    while ((newlineIdx = dataBuffer.indexOf('\n')) !== -1) {
        const line = dataBuffer.substring(0, newlineIdx).trim();
        dataBuffer = dataBuffer.substring(newlineIdx + 1);

        if (line) {
            console.log(`[C OUTPUT]: ${line}`);
            // Resolve the oldest matching request
            if (responseQueue.length > 0) {
                const { resolve } = responseQueue.shift();
                try {
                    const json = JSON.parse(line);
                    resolve(json);
                } catch (e) {
                    console.error('Failed to parse JSON:', e, 'Line:', line);
                    resolve({ error: 'Invalid JSON from C backend', raw: line });
                }
            }
        }
    }
});

dsaProcess.stderr.on('data', (data) => {
    console.error(`[C ERROR]: ${data}`);
});

// Helper to send command and wait for response
function sendCommand(cmd) {
    return new Promise((resolve, reject) => {
        responseQueue.push({ resolve, reject });
        console.log(`[SENDING CMD]: ${cmd}`);
        dsaProcess.stdin.write(cmd + '\n');
    });
}

// --- API ENDPOINTS ---

app.get('/api/stocks', async (req, res) => {
    const data = await sendCommand('STOCKS');
    res.json(data);
});

app.post('/api/stocks', async (req, res) => {
    const { name, buyPrice, quantity } = req.body;
    if (!name || !buyPrice || !quantity) return res.status(400).json({ error: 'Missing fields' });

    const data = await sendCommand(`ADD ${name} ${buyPrice} ${quantity}`);
    res.json(data);
});

app.post('/api/price', async (req, res) => {
    const { name, newPrice } = req.body;
    if (!name || newPrice === undefined) return res.status(400).json({ error: 'Missing fields' });

    const data = await sendCommand(`UPDATE ${name} ${newPrice}`);
    res.json(data);
});

app.get('/api/summary', async (req, res) => {
    const data = await sendCommand('SUMMARY');
    res.json(data);
});

app.get('/api/top', async (req, res) => {
    const data = await sendCommand('TOP');
    res.json(data);
});

app.get('/api/trends/:name', async (req, res) => {
    const data = await sendCommand(`TRENDS ${req.params.name}`);
    res.json(data);
});

app.get('/api/transactions', async (req, res) => {
    const data = await sendCommand('TRANSACTIONS');
    res.json(data);
});

app.listen(PORT, () => {
    console.log(`Backend running on http://localhost:${PORT}`);
});
