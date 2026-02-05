# DSA Stock Portfolio Manager - Web Edition

This project integrates a **C-based DSA engine** (Hash Maps, AVL Trees, Fenwick Trees, Heaps) with a modern **Next.js** frontend.

## 🚀 Quick Start

### 1. Compile the C Engine
The core logic resides in `dsa2.c`. You must compile it first.

```bash
gcc dsa2.c -o dsa2
```
*Note: Make sure `dsa2.exe` is created in this folder.*

### 2. Start the Node.js Backend
This middleware spawns the C program and exposes a JSON API.

```bash
cd backend
npm install
node server.js
```
*Server will start on `http://localhost:5000`*

### 3. Start the Frontend
The modern dashboard to interact with the system.

```bash
# Open a new terminal
cd frontend
npm run dev
```
*Open `http://localhost:3000` in your browser.*

---

## 🏗 Architecture
1.  **C Backend (`dsa2.c`)**: Handles all data structures (AVL, Heaps) and math. Ran in `--api` mode.
2.  **Node.js (`server.js`)**: Connects to C via stdin/stdout, parses JSON, and serves REST endpoints.
3.  **Next.js (`frontend/`)**: React-based UI with Tailwind CSS.

## 🧪 What to Demonstrate
1.  **Add Stock**: Adds to Hash Table and AVL Tree.
2.  **Update Price**: Updates Circular Buffer and Fenwick Trees.
3.  **Dashboard**: Shows `Top Gainer/Loser` (retrieved from Heaps in O(1)).
4.  **Market Overview**: Shows sorted list (AVL In-Order Traversal).
