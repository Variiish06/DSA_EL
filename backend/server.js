const express = require('express');
const { spawn } = require('child_process');
const bodyParser = require('body-parser');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 5000;

app.use(cors());
app.use(bodyParser.json());

const dsaExecutable = process.platform === 'win32' ? 'dsa2.exe' : 'dsa2';
const dsaPath = path.join(__dirname, '..', dsaExecutable);

// --- RESILIENT COMMUNICATION LAYER ---
let isCProcessing = false;
let commandQueue = [];
let dataBuffer = '';
let responseQueue = [];

function spawnCProcess() {
    console.log(`Spawning C process at: ${dsaPath}`);
    const process = spawn(dsaPath, ['--api']);

    process.on('error', (err) => {
        console.error('Failed to start C process:', err);
    });

    process.on('close', (code) => {
        console.log(`C process exited with code ${code}. Restarting...`);
        // Clear queue on crash to avoid hanging requests
        while (responseQueue.length > 0) {
            const { reject } = responseQueue.shift();
            reject(new Error('C process crashed'));
        }
        isCProcessing = false;
        setTimeout(() => { dsaProcess = spawnCProcess(); }, 1000);
    });

    process.stdout.on('data', (data) => {
        dataBuffer += data.toString();
        let newlineIdx;
        while ((newlineIdx = dataBuffer.indexOf('\n')) !== -1) {
            const line = dataBuffer.substring(0, newlineIdx).trim();
            dataBuffer = dataBuffer.substring(newlineIdx + 1);

            if (line) {
                console.log(`[C OUTPUT]: ${line}`);
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
                isCProcessing = false;
                processNextCommand();
            }
        }
    });

    process.stderr.on('data', (data) => {
        console.error(`[C ERROR]: ${data}`);
    });

    return process;
}

let dsaProcess = spawnCProcess();

function processNextCommand() {
    if (isCProcessing || commandQueue.length === 0) return;

    isCProcessing = true;
    const { cmd, resolve, reject } = commandQueue.shift();
    responseQueue.push({ resolve, reject });

    try {
        console.log(`[SENDING CMD]: ${cmd}`);
        dsaProcess.stdin.write(cmd + '\n');
    } catch (e) {
        console.error('Failed to write to C stdin:', e);
        const { reject } = responseQueue.shift();
        reject(e);
        isCProcessing = false;
    }
}

function sendCommand(cmd) {
    return new Promise((resolve, reject) => {
        commandQueue.push({ cmd, resolve, reject });
        processNextCommand();
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
    const { name, newPrice, newQty } = req.body;
    if (!name || newPrice === undefined) return res.status(400).json({ error: 'Missing fields' });

    // Ensure qty is a number, default to -1 (C engine should ignore if -1 or 0)
    const qty = newQty !== undefined ? Number(newQty) : -1;
    const data = await sendCommand(`UPDATE ${name} ${newPrice} ${qty}`);
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

app.get('/api/clusters', async (req, res) => {
    const data = await sendCommand('CLUSTERS');
    res.json(data);
});

app.listen(PORT, () => {
    console.log(`Backend running on http://localhost:${PORT}`);
});
