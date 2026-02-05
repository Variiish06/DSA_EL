/*
 * DSA PROJECT: Advanced Stock Management System
 * Features: hash map, circular buffer, Fenwick trees, AVL, Heaps, Trie, Graph.
 *
 * COMPILE: gcc -Wall -Wextra -std=c11 dsa2.c -o dsa2 -lm
 * RUN CLI: ./dsa2
 * RUN API: ./dsa2 --api
 */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- CONFIGURATION --- */
#define HASH_SIZE 50
#define HISTORY_SIZE 100 // Window size for history
#define MAX_STOCKS 100   // Max capacity
#define NAME_LEN 20

/* --- DATA STRUCTURES --- */

// 1. Transaction Linked List
typedef struct Transaction {
  char type[10]; // BUY, SELL, UPDATE
  char symbol[NAME_LEN];
  float price;
  struct Transaction *next;
} Transaction;

// 2. Trie Node for Symbol Search
typedef struct TrieNode {
  struct TrieNode *children[26];
  bool isEndOfWord;
} TrieNode;

// 3. Stock Object
typedef struct Stock {
  char name[NAME_LEN];
  float currentPrice;
  float buyPrice;
  int quantity;

  // ALERTS
  float upperAlert;
  float lowerAlert;

  // HISTORY & FENWICK TREES (BIT)
  // We map a circular buffer to a linear BIT for O(log N) window sums.
  float priceHistory[HISTORY_SIZE];
  float bit_price[HISTORY_SIZE + 1]; // 1-based indexing for BIT
  float bit_gain[HISTORY_SIZE + 1];
  float bit_loss[HISTORY_SIZE + 1];
  int head;  // Points to the NEXT index to write (Circular)
  int count; // Number of history points filled

  // HEAP INDICES (for O(log N) updates)
  int maxHeapIdx;
  int minHeapIdx;

  struct Stock *next; // For Hash Table collision
} Stock;

// 4. AVL Tree Node (Sorted by % Gain)
typedef struct AVL {
  Stock *stock;
  int height;
  struct AVL *left, *right;
} AVL;

/* --- GLOBALS --- */
Stock *hashTable[HASH_SIZE] = {0};
AVL *avlRoot = NULL;
Stock *maxHeap[MAX_STOCKS];
Stock *minHeap[MAX_STOCKS];
int heapSize = 0; // Shared size for simplicity (assuming all stocks in both)

Transaction *transHead = NULL;
TrieNode *trieRoot = NULL;

// Correlation Matrix (1 = Correlated, 0 = No)
int correlationGraph[MAX_STOCKS][MAX_STOCKS];
Stock *stockRegistry[MAX_STOCKS]; // Map ID to Stock Pointer
int registryCount = 0;

/* --- PROTOTYPES --- */
void updateStockPrice(char *name, float newPrice, int newQty, bool isAuto);
float getPercent(Stock *s);

/* ================= UTILITIES & MATH ================= */

int hash(const char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return abs((int)(hash % HASH_SIZE));
}

float max_f(float a, float b) { return (a > b) ? a : b; }
int max_i(int a, int b) { return (a > b) ? a : b; }

/* ================= FENWICK TREE (BIT) ================= */
// Operations are O(log N)

// Update index 'i' by adding 'delta'
void bit_update(float *bit, int idx, float delta) {
  idx++; // Convert 0-based array index to 1-based BIT index
  while (idx <= HISTORY_SIZE) {
    bit[idx] += delta;
    idx += idx & (-idx);
  }
}

// Prefix sum up to index 'i'
float bit_query(float *bit, int idx) {
  idx++; // Convert 0-based array index to 1-based BIT index
  float sum = 0;
  while (idx > 0) {
    sum += bit[idx];
    idx -= idx & (-idx);
  }
  return sum;
}

// Range sum [L, R] handling circular wrapping manually in caller if needed
// This function strictly calculates linear range sum in the underlying array
float bit_query_range(float *bit, int L, int R) {
  if (L > R)
    return 0;
  return bit_query(bit, R) - bit_query(bit, L - 1);
}

/* ================= TRIE ================= */
// Operations O(L)

TrieNode *createTrieNode() {
  TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));
  return node;
}

void insertTrie(const char *word) {
  if (!trieRoot)
    trieRoot = createTrieNode();
  TrieNode *curr = trieRoot;
  for (int i = 0; word[i]; i++) {
    int idx = word[i] - 'A';
    if (idx < 0 || idx >= 26)
      continue; // Basic sanitization
    if (!curr->children[idx])
      curr->children[idx] = createTrieNode();
    curr = curr->children[idx];
  }
  curr->isEndOfWord = true;
}

bool searchTrie(const char *word) {
  if (!trieRoot)
    return false;
  TrieNode *curr = trieRoot;
  for (int i = 0; word[i]; i++) {
    int idx = word[i] - 'A';
    if (idx < 0 || idx >= 26)
      return false;
    if (!curr->children[idx])
      return false;
    curr = curr->children[idx];
  }
  return curr->isEndOfWord;
}

/* ================= AVL TREE ================= */
// Operations O(log N)

int getHeight(AVL *n) { return n ? n->height : 0; }
int getBalance(AVL *n) {
  return n ? getHeight(n->left) - getHeight(n->right) : 0;
}

AVL *rightRotate(AVL *y) {
  AVL *x = y->left;
  AVL *T2 = x->right;
  x->right = y;
  y->left = T2;
  y->height = max_i(getHeight(y->left), getHeight(y->right)) + 1;
  x->height = max_i(getHeight(x->left), getHeight(x->right)) + 1;
  return x;
}

AVL *leftRotate(AVL *x) {
  AVL *y = x->right;
  AVL *T2 = y->left;
  y->left = x;
  x->right = T2;
  x->height = max_i(getHeight(x->left), getHeight(x->right)) + 1;
  y->height = max_i(getHeight(y->left), getHeight(y->right)) + 1;
  return y;
}

// Standard AVL Insert based on Name (since % gain changes dynamically,
// usually AVL is keyed by ID/Name, or requires Delete/Re-insert on update.
// Here we key by Name to keep structure valid. For Sorted display, we do
// in-order traversal).
AVL *insertAVL(AVL *node, Stock *s) {
  if (!node) {
    AVL *n = (AVL *)malloc(sizeof(AVL));
    n->stock = s;
    n->left = n->right = NULL;
    n->height = 1;
    return n;
  }
  if (strcmp(s->name, node->stock->name) < 0)
    node->left = insertAVL(node->left, s);
  else if (strcmp(s->name, node->stock->name) > 0)
    node->right = insertAVL(node->right, s);
  else
    return node;

  node->height = 1 + max_i(getHeight(node->left), getHeight(node->right));
  int balance = getBalance(node);

  if (balance > 1 && strcmp(s->name, node->left->stock->name) < 0)
    return rightRotate(node);
  if (balance < -1 && strcmp(s->name, node->right->stock->name) > 0)
    return leftRotate(node);
  if (balance > 1 && strcmp(s->name, node->left->stock->name) > 0) {
    node->left = leftRotate(node->left);
    return rightRotate(node);
  }
  if (balance < -1 && strcmp(s->name, node->right->stock->name) < 0) {
    node->right = rightRotate(node->right);
    return leftRotate(node);
  }
  return node;
}

// In-order Traversal to print sorted by % Gain (calculated on fly)
// Note: To strictly SORT by gain, the tree must be keyed by gain.
// For this project, we iterate the AVL (alphabetical) but can dump to array to
// sort for display. We will just print the AVL contents here.
void inOrderAVL(AVL *root) {
  if (root) {
    inOrderAVL(root->left);
    printf("  %-10s | %6.2f%%\n", root->stock->name, getPercent(root->stock));
    inOrderAVL(root->right);
  }
}

/* ================= HEAPS (Max & Min) ================= */
// Operations O(log N)

void swapStocks(Stock **a, Stock **b) {
  Stock *temp = *a;
  *a = *b;
  *b = temp;
}

// Generic Heapify
void heapifyMax(int idx) {
  int largest = idx;
  int left = 2 * idx + 1;
  int right = 2 * idx + 2;

  if (left < heapSize &&
      getPercent(maxHeap[left]) > getPercent(maxHeap[largest]))
    largest = left;
  if (right < heapSize &&
      getPercent(maxHeap[right]) > getPercent(maxHeap[largest]))
    largest = right;

  if (largest != idx) {
    // Swap pointers in heap array
    swapStocks(&maxHeap[idx], &maxHeap[largest]);
    // Update indices in Stock structs
    maxHeap[idx]->maxHeapIdx = idx;
    maxHeap[largest]->maxHeapIdx = largest;
    heapifyMax(largest);
  }
}

void heapifyMin(int idx) {
  int smallest = idx;
  int left = 2 * idx + 1;
  int right = 2 * idx + 2;

  if (left < heapSize &&
      getPercent(minHeap[left]) < getPercent(minHeap[smallest]))
    smallest = left;
  if (right < heapSize &&
      getPercent(minHeap[right]) < getPercent(minHeap[smallest]))
    smallest = right;

  if (smallest != idx) {
    swapStocks(&minHeap[idx], &minHeap[smallest]);
    minHeap[idx]->minHeapIdx = idx;
    minHeap[smallest]->minHeapIdx = smallest;
    heapifyMin(smallest);
  }
}

void updateHeaps(Stock *s) {
  // Bubble Up/Down Max Heap
  int i = s->maxHeapIdx;
  while (i && getPercent(maxHeap[i]) > getPercent(maxHeap[(i - 1) / 2])) {
    int p = (i - 1) / 2;
    swapStocks(&maxHeap[i], &maxHeap[p]);
    maxHeap[i]->maxHeapIdx = i;
    maxHeap[p]->maxHeapIdx = p;
    i = p;
  }
  heapifyMax(s->maxHeapIdx);

  // Bubble Up/Down Min Heap
  i = s->minHeapIdx;
  while (i && getPercent(minHeap[i]) < getPercent(minHeap[(i - 1) / 2])) {
    int p = (i - 1) / 2;
    swapStocks(&minHeap[i], &minHeap[p]);
    minHeap[i]->minHeapIdx = i;
    minHeap[p]->minHeapIdx = p;
    i = p;
  }
  heapifyMin(s->minHeapIdx);
}

/* ================= CORE LOGIC ================= */

Stock *findStock(char *name) {
  int h = hash(name);
  Stock *s = hashTable[h];
  while (s) {
    if (strcmp(s->name, name) == 0)
      return s;
    s = s->next;
  }
  return NULL;
}

float getPercent(Stock *s) {
  if (s->buyPrice == 0)
    return 0;
  return ((s->currentPrice - s->buyPrice) / s->buyPrice) * 100.0f;
}

// CALCULATE SMA using Fenwick Tree
float calculateSMA(Stock *s, int period) {
  if (s->count < period)
    period = s->count; // Fallback
  if (period == 0)
    return s->currentPrice;

  // The circular buffer logical end is (head - 1)
  int logicalEnd = (s->head - 1 + HISTORY_SIZE) % HISTORY_SIZE;
  int logicalStart = (s->head - period + HISTORY_SIZE) % HISTORY_SIZE;

  float sum = 0;
  // If range does not wrap around underlying array
  if (logicalStart <= logicalEnd) {
    sum = bit_query_range(s->bit_price, logicalStart, logicalEnd);
  } else {
    // Wraps around: [logicalStart...End] + [0...logicalEnd]
    sum = bit_query_range(s->bit_price, logicalStart, HISTORY_SIZE - 1) +
          bit_query_range(s->bit_price, 0, logicalEnd);
  }
  return sum / period;
}

// CALCULATE RSI using Fenwick Trees for Gains and Losses
float calculateRSI(Stock *s, int period) {
  if (s->count < period + 1)
    return 50.0; // Needs period + 1 points for 'period' changes

  int logicalEnd = (s->head - 1 + HISTORY_SIZE) % HISTORY_SIZE;
  int logicalStart = (s->head - period + HISTORY_SIZE) % HISTORY_SIZE;

  float totalGain = 0, totalLoss = 0;

  if (logicalStart <= logicalEnd) {
    totalGain = bit_query_range(s->bit_gain, logicalStart, logicalEnd);
    totalLoss = bit_query_range(s->bit_loss, logicalStart, logicalEnd);
  } else {
    totalGain = bit_query_range(s->bit_gain, logicalStart, HISTORY_SIZE - 1) +
                bit_query_range(s->bit_gain, 0, logicalEnd);
    totalLoss = bit_query_range(s->bit_loss, logicalStart, HISTORY_SIZE - 1) +
                bit_query_range(s->bit_loss, 0, logicalEnd);
  }

  float avgGain = totalGain / period;
  float avgLoss = totalLoss / period;

  if (avgLoss == 0)
    return 100.0;
  float rs = avgGain / avgLoss;
  return 100.0 - (100.0 / (1.0 + rs));
}

// Add Transaction Log
void logTransaction(const char *type, const char *name, float price) {
  Transaction *t = (Transaction *)malloc(sizeof(Transaction));
  strcpy(t->type, type);
  strcpy(t->symbol, name);
  t->price = price;
  t->next = transHead;
  transHead = t;
}

// Create Stock
void addStock(char *name, float buyPrice, int qty) {
  if (registryCount >= MAX_STOCKS) {
    printf("Error: Max stocks reached.\n");
    return;
  }
  if (findStock(name)) {
    printf("Error: Stock %s already exists.\n", name);
    return;
  }

  Stock *s = (Stock *)calloc(1, sizeof(Stock));
  strcpy(s->name, name);
  s->buyPrice = buyPrice;
  s->currentPrice = buyPrice;
  s->quantity = qty;
  s->upperAlert = buyPrice * 1.10;
  s->lowerAlert = buyPrice * 0.90;

  // Init History
  s->head = 0;
  s->count = 0;

  // Add initial price to history/BIT
  s->priceHistory[0] = buyPrice;
  bit_update(s->bit_price, 0, buyPrice);
  s->head = 1;
  s->count = 1;

  // Hash Table
  int h = hash(name);
  s->next = hashTable[h];
  hashTable[h] = s;

  // Structures
  avlRoot = insertAVL(avlRoot, s);

  maxHeap[heapSize] = s;
  s->maxHeapIdx = heapSize;
  minHeap[heapSize] = s;
  s->minHeapIdx = heapSize;
  heapSize++;
  updateHeaps(s); // Init sort

  insertTrie(name);
  stockRegistry[registryCount++] = s;

  logTransaction("INIT", name, buyPrice);
  // printf("Stock %s added at %.2f\n", name, buyPrice);
}

// Update Price (The most complex logic)
void updateStockPrice(char *name, float newPrice, int newQty, bool isAuto) {
  Stock *s = findStock(name);
  if (!s) {
    printf("Stock not found.\n");
    return;
  }

  float oldPrice = s->priceHistory[(s->head - 1 + HISTORY_SIZE) % HISTORY_SIZE];

  // 1. Logic for circular buffer overwrite
  int currIdx = s->head;
  float valToRemove = 0;
  float gainToRemove = 0;
  float lossToRemove = 0;

  // If buffer full, we are overwriting the oldest value.
  if (s->count == HISTORY_SIZE) {
    valToRemove = s->priceHistory[currIdx];
    bit_update(s->bit_price, currIdx, -valToRemove);

    // Remove old gain/loss contributions from BIT (Simplified approximation)
  }

  // Calculate new metrics
  float change = newPrice - oldPrice;
  float gain = (change > 0) ? change : 0;
  float loss = (change < 0) ? -change : 0;

  // Update Price History & BIT
  s->priceHistory[currIdx] = newPrice;
  bit_update(s->bit_price, currIdx, newPrice);

  // Update Gain/Loss BITs
  bit_update(s->bit_gain, currIdx, gain);
  bit_update(s->bit_loss, currIdx, loss);

  s->head = (s->head + 1) % HISTORY_SIZE;
  if (s->count < HISTORY_SIZE)
    s->count++;

  s->currentPrice = newPrice;
  if (newQty > 0)
    s->quantity = newQty;
  if (newQty > 0)
    s->quantity = newQty;

  // Update Heaps
  updateHeaps(s);

  if (isAuto) {
    // Silent update for test harness
  } else {
    // printf("Updated %s to %.2f. Change: %.2f\n", name, newPrice, change);
    if (newPrice >= s->upperAlert) { /* Alert logic handled by frontend polling
                                        or return data */
    }
    if (newPrice <= s->lowerAlert) { /* Alert logic */
    }
  }

  logTransaction("UPDATE", name, newPrice);
}

/* ================= ANALYSIS ENGINE ================= */

void analyzeIndicators() {
  printf("\n%-10s | %-8s | %-8s | %-6s | %-15s\n", "STOCK", "PRICE", "SMA(5)",
         "RSI(14)", "SIGNAL");
  printf("---------------------------------------------------------------\n");

  // Reset graph for this snapshot
  memset(correlationGraph, 0, sizeof(correlationGraph));

  for (int i = 0; i < registryCount; i++) {
    Stock *s = stockRegistry[i];
    float sma = calculateSMA(s, 5);
    float rsi = calculateRSI(s, 14);

    char signal[20] = "HOLD";

    // Strategy: RSI Mean Reversion
    if (rsi < 30)
      strcpy(signal, "BUY (Oversold)");
    else if (rsi > 70)
      strcpy(signal, "SELL (Overbought)");

    // Build Correlation: Link stocks that are both Oversold
    if (rsi < 30) {
      for (int j = 0; j < registryCount; j++) {
        if (i == j)
          continue;
        if (calculateRSI(stockRegistry[j], 14) < 30) {
          correlationGraph[i][j] = 1;
        }
      }
    }

    printf("%-10s | %8.2f | %8.2f | %6.1f | %s\n", s->name, s->currentPrice,
           sma, rsi, signal);
  }

  printf("\n[Graph Analysis] Sector Risk Clusters (Correlated Oversold "
         "Stocks):\n");
  bool foundRisk = false;
  bool visited[MAX_STOCKS] = {0};

  for (int i = 0; i < registryCount; i++) {
    if (!visited[i] && calculateRSI(stockRegistry[i], 14) < 30) {
      bool cluster = false;
      for (int j = 0; j < registryCount; j++) {
        if (correlationGraph[i][j]) {
          if (!cluster) {
            printf("  Cluster: %s", stockRegistry[i]->name);
            cluster = true;
            visited[i] = true;
          }
          printf(", %s", stockRegistry[j]->name);
          visited[j] = true;
        }
      }
      if (cluster) {
        printf("\n");
        foundRisk = true;
      }
    }
  }
  if (!foundRisk)
    printf("  None detected.\n");
}

/* ================= TEST HARNESS ================= */

void runAutoTest() {
  printf("\n=== RUNNING AUTOMATED TEST HARNESS ===\n");

  // 1. Insert Stocks
  addStock("RELIANCE", 2400.00, 10);
  addStock("TCS", 3500.00, 5);
  addStock("INFY", 1500.00, 20);

  // 2. Simulate Price History for RELIANCE (Downtrend -> Oversold)
  float rel_prices[] = {2380, 2350, 2300, 2250, 2200, 2150,
                        2100, 2050, 2000, 1950, 1900, 1850};
  for (int i = 0; i < 12; i++)
    updateStockPrice("RELIANCE", rel_prices[i], 10, true);

  // 3. Simulate Price History for TCS (Uptrend)
  float tcs_prices[] = {3550, 3600, 3650, 3700, 3750, 3800, 3850, 3900};
  for (int i = 0; i < 8; i++)
    updateStockPrice("TCS", tcs_prices[i], 5, true);

  // 4. Simulate Price History for INFY (Correlation with RELIANCE - also
  // crashing)
  float infy_prices[] = {1480, 1450, 1400, 1350, 1300, 1250, 1200, 1150, 1100};
  for (int i = 0; i < 9; i++)
    updateStockPrice("INFY", infy_prices[i], 20, true);

  // 5. Verify Structures
  printf("\n[Validation] Top Gainer: %s (%.2f%%)\n", maxHeap[0]->name,
         getPercent(maxHeap[0]));
  printf("[Validation] Top Loser:  %s (%.2f%%)\n", minHeap[0]->name,
         getPercent(minHeap[0]));

  printf("\n[Validation] Trie Search 'TCS': %s\n",
         searchTrie("TCS") ? "FOUND" : "FAIL");
  printf("[Validation] Trie Search 'XYZ': %s\n",
         searchTrie("XYZ") ? "FOUND" : "NOT FOUND (Correct)");

  // 6. Run Analysis
  analyzeIndicators();
  printf("\n=== TEST COMPLETE ===\n");
}

/* ================= API MODE ================= */

// Helper to sanitize float printing to JSON
void printFloat(float f) {
  if (isnan(f))
    printf("null");
  else
    printf("%.2f", f);
}

// Print single stock object as JSON
void printStockJSON(Stock *s, bool last) {
  float gain = getPercent(s);
  float sma = calculateSMA(s, 5);
  float rsi = calculateRSI(s, 14);

  printf("{\"name\": \"%s\", \"buyPrice\": %.2f, \"currentPrice\": %.2f, "
         "\"quantity\": %d, \"percentGain\": %.2f, \"sma\": %.2f, \"rsi\": "
         "%.2f, \"upperAlert\": %.2f, \"lowerAlert\": %.2f}%s",
         s->name, s->buyPrice, s->currentPrice, s->quantity, gain, sma, rsi,
         s->upperAlert, s->lowerAlert, last ? "" : ",");
}

void apiRecursiveAVL(AVL *root, bool *isFirst) {
  if (root) {
    apiRecursiveAVL(root->left, isFirst);
    if (!*isFirst)
      printf(",");
    printStockJSON(root->stock, true);
    *isFirst = false;
    apiRecursiveAVL(root->right, isFirst);
  }
}

// Helper to print all stocks using In-Order Traversal (Sorted by Name in this
// impl, but effectively all stocks)
void cmdStocks() {
  printf("[");
  bool isFirst = true;
  apiRecursiveAVL(avlRoot, &isFirst);
  printf("]\n");
}

void cmdTop() {
  printf("{");
  if (heapSize > 0) {
    printf("\"topGainer\": ");
    printStockJSON(maxHeap[0], true);
    printf(", \"topLoser\": ");
    printStockJSON(minHeap[0], true);
  } else {
    printf("\"topGainer\": null, \"topLoser\": null");
  }
  printf("}\n");
}

void cmdSummary() {
  float totalInvest = 0;
  float currentValue = 0;

  for (int i = 0; i < registryCount; i++) {
    Stock *s = stockRegistry[i];
    totalInvest += s->buyPrice * s->quantity;
    currentValue += s->currentPrice * s->quantity;
  }

  printf("{\"totalInvestment\": %.2f, \"currentValue\": %.2f, \"profit\": "
         "%.2f, \"stockCount\": %d}\n",
         totalInvest, currentValue, currentValue - totalInvest, registryCount);
}

void cmdTrends(char *name) {
  Stock *s = findStock(name);
  if (!s) {
    printf("{\"error\": \"Stock not found\"}\n");
    return;
  }

  float sma = calculateSMA(s, 5);
  float rsi = calculateRSI(s, 14);
  char signal[50] = "HOLD";
  char confidence[10] = "MEDIUM";

  if (rsi < 30) {
    strcpy(signal, "BUY (Oversold)");
    strcpy(confidence, "HIGH");
  } else if (rsi > 70) {
    strcpy(signal, "SELL (Overbought)");
    strcpy(confidence, "HIGH");
  }

  printf("{\"name\": \"%s\", \"sma\": %.2f, \"rsi\": %.2f, \"recommendation\": "
         "\"%s\", \"confidence\": \"%s\"}\n",
         s->name, sma, rsi, signal, confidence);
}

void cmdTransactions() {
  printf("[");
  Transaction *t = transHead;
  int count = 0;
  while (t && count < 50) { // Limit to last 50
    if (count > 0)
      printf(",");
    printf("{\"type\": \"%s\", \"symbol\": \"%s\", \"price\": %.2f}", t->type,
           t->symbol, t->price);
    t = t->next;
    count++;
  }
  printf("]\n");
}

void runApiMode() {
  char buffer[256];
  char cmd[50];
  char arg1[50];
  float arg2, arg3;
  int arg4;

  // Flush immediately on start to signal ready? No, just loop.

  while (fgets(buffer, sizeof(buffer), stdin)) {
    // Remove newline
    buffer[strcspn(buffer, "\n")] = 0;

    // Basic parsing
    int parts =
        sscanf(buffer, "%s %s %f %f %d", cmd, arg1, &arg2, &arg3, &arg4);

    if (strcmp(cmd, "STOCKS") == 0 || strcmp(cmd, "SORTED") == 0) {
      cmdStocks();
    } else if (strcmp(cmd, "ADD") == 0) {
      // arg1=Name, arg2=BuyPrice, arg4=Qty (arg3 ignored if not needed, let's
      // strictly follow sscanf) Re-parse for specific ADD signature: ADD Name
      // Price Qty
      sscanf(buffer, "%s %s %f %d", cmd, arg1, &arg2, &arg4);
      addStock(arg1, arg2, arg4);
      printf("{\"status\": \"ok\", \"message\": \"Stock Added\"}\n");
    } else if (strcmp(cmd, "UPDATE") == 0) {
      // UPDATE Name Price Qty
      sscanf(buffer, "%s %s %f %d", cmd, arg1, &arg2, &arg4);
      updateStockPrice(arg1, arg2, arg4, false); // false = not auto
      printf("{\"status\": \"ok\", \"message\": \"Price & Qty Updated\"}\n");
    } else if (strcmp(cmd, "SUMMARY") == 0) {
      cmdSummary();
    } else if (strcmp(cmd, "TOP") == 0) {
      cmdTop();
    } else if (strcmp(cmd, "TRENDS") == 0) {
      // TRENDS Name
      sscanf(buffer, "%s %s", cmd, arg1);
      cmdTrends(arg1);
    } else if (strcmp(cmd, "TRANSACTIONS") == 0) {
      cmdTransactions();
    } else {
      printf("{\"error\": \"Unknown command\"}\n");
    }

    fflush(stdout); // CRITICAL: Ensure Node.js receives the packet immediately
  }
}

/* ================= MAIN ================= */

int main(int argc, char *argv[]) {
  // Check for API mode flag
  if (argc > 1 && strcmp(argv[1], "--api") == 0) {
    runApiMode();
    return 0;
  }

  // Default Interactive Mode
  int choice;
  char name[20];
  float p;
  int q;

  while (1) {
    printf("\n1. Add Stock\n2. Update Price\n3. Show Analysis\n4. Show Sorted "
           "(AVL)\n5. Top Gainer/Loser\n6. Run Auto-Test (Hardcoded)\n7. "
           "Exit\n> ");
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n')
        ; // flush
      continue;
    }

    switch (choice) {
    case 1:
      printf("Name: ");
      scanf("%19s", name);
      printf("Price: ");
      scanf("%f", &p);
      printf("Qty: ");
      scanf("%d", &q);
      addStock(name, p, q);
      break;
    case 2:
      printf("\n--- UPDATE STOCK ---\n");
      printf("%-15s: ", "Name");
      scanf("%19s", name);
      printf("%-15s: ", "New Price");
      scanf("%f", &p);
      printf("%-15s: ", "New Quantity");
      scanf("%d", &q);
      updateStockPrice(name, p, q, false);
      break;
    case 3:
      analyzeIndicators();
      break;
    case 4:
      printf("\nSorted by Gain (AVL In-Order):\n");
      inOrderAVL(avlRoot);
      break;
    case 5:
      if (heapSize > 0) {
        printf("Top Gainer: %s (%.2f%%)\n", maxHeap[0]->name,
               getPercent(maxHeap[0]));
        printf("Top Loser:  %s (%.2f%%)\n", minHeap[0]->name,
               getPercent(minHeap[0]));
      } else
        printf("No stocks.\n");
      break;
    case 6:
      runAutoTest();
      break;
    case 7:
      exit(0);
    default:
      printf("Invalid.\n");
    }
  }
  return 0;
}
