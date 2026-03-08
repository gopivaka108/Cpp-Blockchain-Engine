#include <iostream>
#include <queue>
#include <vector>
#include <string>

using namespace std;

// Step 1: Define the transaction structure
struct Transaction {
    string txId;
    string sender;
    string receiver;
    double amount;
    double gasPrice; // Validators prioritize high Gas

    // Overload the < operator so the priority queue acts as a max-heap sorted by Gas
    bool operator<(const Transaction& other) const {
        return gasPrice < other.gasPrice;
    }
};

int main() {
    cout << "Starting Mempool Monitor..." << endl;
    
    // Create the mempool
    priority_queue<Transaction> mempool;

    // Step 2: Add dummy transactions
    // Format: txId, sender, receiver, amount, gasPrice
    mempool.push({"tx001", "Alice", "DEX", 1.5, 45.0});
    mempool.push({"tx002", "Bob", "DEX", 15.0, 120.0}); // Huge trade, high gas!
    mempool.push({"tx003", "Charlie", "DEX", 0.5, 30.0});

    cout << "Added 3 transactions to the Mempool Monitor." << endl;

    // Let's print the top transaction to prove it sorted correctly
    Transaction topTx = mempool.top();
    cout << "Next transaction to process is: " << topTx.txId 
         << " with Gas: " << topTx.gasPrice << endl;

    // --- Step 3: Block Execution & Attack Logic ---
    vector<Transaction> blockExecution;
    bool attackExecuted = false;

    cout << "\n--- Miner is building the block ---\n";

    // Loop through the mempool (highest gas gets pulled first)
    while (!mempool.empty()) {
        Transaction currentTx = mempool.top();
        mempool.pop();

        // The Scanner: Look for a massive DEX trade to front-run
        if (!attackExecuted && currentTx.amount >= 10.0 && currentTx.receiver == "DEX") {
            cout << "[!] Target spotted: " << currentTx.txId 
                 << " paying Gas: " << currentTx.gasPrice << endl;

            // The Attack: Create a transaction with slightly higher gas
            Transaction attackTx = {"tx_hacker", "Attacker", "DEX", currentTx.amount, currentTx.gasPrice + 1.0};
            
            cout << "[!] Injecting front-run transaction paying Gas: " << attackTx.gasPrice << "\n" << endl;

            // Push the attacker's transaction into the block FIRST
            blockExecution.push_back(attackTx);
            attackExecuted = true;
        }

        // Push the original transaction into the block AFTER ours
        blockExecution.push_back(currentTx);
    }

    // --- Step 4: Prove the Attack Worked ---
    cout << "--- Final Block Execution Order ---\n";
    for (const auto& tx : blockExecution) {
        cout << "Executed: " << tx.txId << " | Sender: " << tx.sender 
             << " | Gas: " << tx.gasPrice << endl;
    }
    
    return 0;
}