#include <iostream>
#include <fstream>
#include <string>
#include <climits>
// #include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <set>
#include <queue>
#include <regex> 
#include <string>
#include <sstream>
#include <map>
#include <unordered_map>
#include<bits/stdc++.h>

using namespace std;

/*class site{
    int site_id;
    bool is_working;
    set<int>variables;
    int last_commit = 0;
};*/

//vector<site>site_data();

// Contains Correct data at current time curr_t for all committed transactions.


class operation{
    public:
    int transaction_id;
    int timestamp;
    char op_type;
    int variable_id; 
    int site_id;
    int val;
    set<int>wait_site_ids;
    operation() : wait_site_ids(set<int>()) {}
};

class Transaction{
    public:
    int transaction_id;
    int start_timestamp;
    int commit_timestamp;
    /*set<int>sites_written;
    vector<int>reads;
    vector<int>writes;
    bool aborted;
    set<int>datavals;*/
    vector<operation> operations;
    bool aborted;
    vector<pair<int,vector<int>>>data_vals;
    vector<int> read_vars;
    vector<vector<bool>> can_read;

    Transaction() : transaction_id(0), start_timestamp(0), commit_timestamp(0), aborted(false),data_vals(20, make_pair(INT_MAX,vector<int>())), can_read(10,vector<bool> (20,true)) {}
    Transaction(int id, int timestamp) 
        : transaction_id(id), start_timestamp(timestamp), aborted(false),data_vals(20, make_pair(INT_MAX,vector<int>())), can_read(10,vector<bool> (20,true)) {}    
};

vector<Transaction>transactions_data;

class DataManager {
public:
    
    map<int, vector<vector<int>>> main_data;
    vector<operation> commit_logs;
    vector<int> committed_timestamps;
    vector<pair<int,vector<int>>> last_commit;
    vector<vector<pair<int,vector<int>>>> all_commits;
    vector<set<int>> read_trans,write_trans;
    static DataManager* instance;
    DataManager() {
        // Initialize with initial values
        //vector<vector<int>> initial_data(10, vector<int>(20, 0));
        //main_data[0] = initial_data;
        committed_timestamps.push_back(0);
        last_commit.resize(20,pair<int,vector<int>> ());
        read_trans.resize(20,set<int>());
        write_trans.resize(20,set<int>());
    }

    static DataManager* getInstance() {
        if (instance == nullptr) {
            instance = new DataManager();
        }
        return instance;
    }

    int readval(int timestamp, int site_id, int var_id) {
        // 1. Do binary search of all committed timestamps to find the timestamp lesser than 
        // the given timestamp.
        // 2. Retrieve the value from main_data for the output timestamp
       if(committed_timestamps.size() <= 1) {
            return (var_id + 1) * 10;  // Initial value based on variable id
        }

        auto it = upper_bound(committed_timestamps.begin(), committed_timestamps.end(), timestamp);
        if(it == committed_timestamps.begin()) {
            return (var_id + 1) * 10;  // Return initial value
        }
        --it;
        //cout << *it << " " << site_id << " " << endl;
        return main_data[*it][site_id][var_id];
    }

    pair<int,vector<int>> get_last_commit(int timestamp,int var_id){
        pair<int,vector<int>> last_commit_b4_timestamp_sites;
        int return_ts = 0;
        for(auto vec:all_commits){
            if(vec[var_id].first <= timestamp){
                return_ts = vec[var_id].first;
                last_commit_b4_timestamp_sites = vec[var_id];
            }
            else{
                break;
            }
        }
        
        return last_commit_b4_timestamp_sites;
    }

    void append_commit(vector<operation>& ops, int timestamp, vector<vector<int>>& current_data) {
        commit_logs.insert(commit_logs.end(), ops.begin(), ops.end());
        committed_timestamps.push_back(timestamp);
        main_data[timestamp] = current_data;
    }
};

DataManager* DataManager::instance = nullptr;

class SiteManager {
public:
    vector<bool> site_status;
    vector<vector<int>> fail_logs;
    vector<vector<int>> site_data;
    vector<vector<int>> site_var_map;
    vector<vector<int>> var_site_map;
    vector<vector<bool>> can_read;      // Can't read from any recovered site until commit on that data item
    static SiteManager* instance;
    vector<vector<int>>trans_site_write;

    static SiteManager* getInstance() {
        if (instance == nullptr) {
            instance = new SiteManager();
            instance->initialize();
        }
        return instance;
    }

    SiteManager() 
        : site_status(10, true),
          fail_logs(10, vector<int>()), 
          site_data(10, vector<int>(20, INT_MAX)),
          site_var_map(20, vector<int>()),
          var_site_map(10, vector<int>()),
          can_read(10,vector<bool> (20,true)),
          trans_site_write(10,vector<int>())
           {}

    void initialize() {
        for(int i = 1; i <= 20; i++) {
            DataManager::getInstance()->last_commit[i-1].first = 0;
            if(i % 2 == 0) {
                for(int site = 0; site < 10; site++) {
                    site_var_map[i-1].push_back(site);
                    var_site_map[site].push_back(i-1);
                    site_data[site][i-1] = i * 10;
                    DataManager::getInstance()->last_commit[i-1].second.push_back(site);
                }
            } else {
                int site = (i % 10);
                site_var_map[i-1].push_back(site);
                var_site_map[site].push_back(i-1);
                site_data[site][i-1] = i * 10;
                DataManager::getInstance()->last_commit[i-1].second.push_back(site);
            }
        }
        DataManager::getInstance()->main_data[0] = site_data;
        DataManager::getInstance()->all_commits.push_back(DataManager::getInstance()->last_commit);
    }

    void failsite(int fail_timestamp,int siteno) {
        site_status[siteno] = false;
        cout << "Site " << siteno+1 << " Failure" << endl;
        fail_logs[siteno].push_back(fail_timestamp);
        for(int i=0;i<trans_site_write[siteno].size();i++){
            cout << "Site "<< siteno+1 << " failed after write so aborting t_id " << trans_site_write[siteno][i] << endl;
            transactions_data[trans_site_write[siteno][i]].aborted  = true;
        }
        // TODO : Notify Transaction Manager
    }

    bool checksite(int siteno) {
        return site_status[siteno];
    }

    int getsite(int variable_id, int transaction_id,vector<vector<bool>> &can_read) {
        Transaction* trans = &transactions_data[transaction_id];
        for(int i = 0; i < site_var_map[variable_id].size(); i++) {
            int site = site_var_map[variable_id][i];
           // cout << site_status[site] << " " << can_read[site][variable_id] << endl;
            if(site_status[site] && can_read[site][variable_id]) return site;
        }
        // All sites are down or not initialized anywhere(this wont happen, we initialize at every site where 
        // value is to be stored).
        // Notify transaction manager for transaction to wait
        if(site_var_map[variable_id].size() > 1) {
            trans->aborted = true;
            cout << "aborted since all sites down " << transaction_id <<  endl;
            return INT_MAX;
        }
        return -1;
    }

    void update_all_sites(int var_id, int val, int transaction_id) {
        for(int site : site_var_map[var_id]) {
            if(site_status[site]) {
                //site_data[site][var_id] = val;
                trans_site_write[site].push_back(transaction_id);
            }
        }
        // TODO if all sites are down shopuld we wait?
        // Should Abort
    }

    vector<int> write_all_sites(int transaction_id,int var_id, int val) {
        vector<int> sites = {};
        for(int site : transactions_data[transaction_id].data_vals[var_id].second) {
            if(site_status[site]) {
                site_data[site][var_id] = val;
                sites.push_back(site);
            }
        }
        // TODO if all sites are down shopuld we wait?
        // Should Abort
    }

    void recover_site(int site_id) {
        site_status[site_id] = true;
        cout << "Site " << site_id+1 << " recovered" << endl;
    }
};

SiteManager* SiteManager::instance = nullptr;

void dump(){
    cout<<"Printing Dump"<<endl;
    // for(int i=0;i<20;i++){
    //     cout << i << " " ;
    //     int site = SiteManager::getInstance()->getsite(i,0);
    //    cout << (DataManager::getInstance()->main_data[DataManager::getInstance()->committed_timestamps[DataManager::getInstance()->committed_timestamps.size()-1]-1][site][i]) << endl;
    //    //cout << DataManager::getInstance()->main_data[0][site][i] << endl;
    // }
    for(int i=0;i<10;i++){
        cout<<"Site "<<i+1<<" - ";
        for(int j=0;j<20;j++){
            if(SiteManager::getInstance()->site_data[i][j] != INT_MAX){
                cout<<"x"<<j+1<<": "<<SiteManager::getInstance()->site_data[i][j]<<", ";
            }
        }
        cout<<endl;
    }
}


class TransactionManager{
    public:
    vector<operation> all_operations;         
    unordered_map<int, Transaction> transactions;
    vector<set<int>> rw_graph;
    int timestamp = 0; 
    static TransactionManager* instance;
    set<int>committed_nodes;
    vector<operation>wait_operations;

    static TransactionManager* getInstance() {
        if (instance == nullptr) {
            instance = new TransactionManager();
        }
        return instance;
    }

    TransactionManager() : timestamp(0), rw_graph(10000, set<int>()), committed_nodes(set<int>()),wait_operations(vector<operation>()) {}

    void initialize() {
        all_operations.clear();
        transactions.clear();
        timestamp = 0;
    }

    void beginTransaction(int transaction_id, int timestamp) {
        cout << "Started transaction "<< transaction_id <<" at time "<<timestamp << endl;
        transactions_data.resize(max(transaction_id + 1, (int)transactions_data.size()));
        transactions_data[transaction_id] = Transaction(transaction_id, timestamp);
        transactions_data[transaction_id].can_read = SiteManager::getInstance()->can_read;
        transactions[transaction_id] = transactions_data[transaction_id];
    }

    void executeInstructions() {
        for (const auto op : all_operations) {
            //cout << "started" << endl;
            switch (op.op_type) {
                case 'R':
                    executeRead(op.timestamp,op.transaction_id, op.variable_id);
                    break;
                case 'W':
                    executeWrite(op.timestamp,op.transaction_id, op.variable_id, op.val);
                    break;
                case 'F':
                    executeFailSite(op.timestamp,op.site_id);
                    break;
                case 'B':
                    beginTransaction(op.transaction_id, op.timestamp); 
                    break;   
                case 'E':
                    executeEnd(op.timestamp,op.transaction_id);
                    break;
                case 'r':
                    executeRecover(op.site_id);  
                    break;
                case 'D':
                    dump();
                    break;
                default :
                    cout << "Invalid operation " << op.op_type << endl;
                    break;      
            }
        }
    }
    bool iscycle(int i, vector<int>&visited,vector<int>&recstack){
        if(visited[i] && recstack[i]) return true;
        recstack[i] = 1;
        visited[i] = 1;
        for(auto j = rw_graph[i].begin();j != rw_graph[i].end();j++){
           if(!visited[*j] && iscycle(*j,visited,recstack)) return true;
           else if (recstack[*j]) return true;
        }

        recstack[i] = 0;
        return false;
    }
    bool dfs(){
        vector<int>visited(1000,0);
        vector<int>recstack(1000,0);
        for(int i=0;i<1000;i++){
            if(!visited[i] && iscycle(i,visited,recstack)) return true;
        }
        return false;
    }
    int executeRead(int timestamp_no, int transaction_id, int var_id){
        Transaction* trans = &transactions_data[transaction_id];
        if(trans->aborted) return INT_MAX;
        // cout << "reading var_id " << var_id+1 << " transaction_id " << transaction_id << endl;
        // Abort if all sites for variable are down between last commit and transn begin
        bool available_site = false;
        if(DataManager::getInstance()->all_commits.size() == 0 ) {
            // cout<<"No commit happened"<<endl;
            available_site = true;
        }
        else {
            pair<int,vector<int>> get_last_commit_for_var = (DataManager::getInstance()->get_last_commit(trans->start_timestamp,var_id));
            // cout<<"For variable "<<var_id+1<<" Commit before "<<(trans->start_timestamp) <<" at "<<get_last_commit_for_var.first<<endl;
            if(get_last_commit_for_var.second.size() == 0){
                // cout<<"HI"<<endl;
                available_site = true;
            }
            for(auto commit_site_id:get_last_commit_for_var.second){
                // cout<<"HI"<<endl;
                // cout<<commit_site_id<<endl;
                if((SiteManager::getInstance()->fail_logs[commit_site_id]).size() != 0){
                    if(SiteManager::getInstance()->fail_logs[commit_site_id][(SiteManager::getInstance()->fail_logs[commit_site_id]).size()-1] < get_last_commit_for_var.first){
                        available_site = true;
                        break;
                    }
                }
                else{
                    // cout<<"Site "<<commit_site_id<<" no fails"<<endl;
                    available_site = true;
                }
            }
        }
        // available_site = true;
        // cout << available_site << endl;
        if(!available_site){
            cout << "Abort if all sites for variable are down between last commit and transn begin" << endl;
            cout<<"Aborted transaction "<<transaction_id<<endl;
            trans->aborted = true;
            return INT_MAX;
        }
        
        // Wait if all required sites are down, pushing transaction atomic executions into a buffer
        operation oper;
        for(int i=0;i<SiteManager::getInstance()->site_var_map[var_id].size();i++){
            if(SiteManager::getInstance()->checksite(SiteManager::getInstance()->site_var_map[var_id][i])) break;

            oper.wait_site_ids.insert(SiteManager::getInstance()->site_var_map[var_id][i]);
            if(i == SiteManager::getInstance()->site_var_map[var_id].size()-1){
                // All sites are down
                oper.op_type = 'R';
                oper.transaction_id = transaction_id;
                //oper.val = val;
                oper.variable_id = var_id;
                oper.timestamp = timestamp_no;
                wait_operations.push_back(oper);
                return INT_MAX;
            }
        }
        DataManager::getInstance()->read_trans[var_id].insert(transaction_id);
        trans->read_vars.push_back(var_id);
        int site = SiteManager::getInstance()->getsite(var_id,transaction_id,trans->can_read);
        // cout << DataManager::getInstance()->readval(transactions_data[transaction_id].start_timestamp,site,var_id) << endl;
        return DataManager::getInstance()->readval(transactions_data[transaction_id].start_timestamp,site,var_id);
    }

    bool can_commit(int transaction_id) {
        Transaction* trans = &transactions_data[transaction_id];
        if(trans->aborted) return false;
        Transaction& t = transactions[transaction_id];
        // Accessed sites are down afetr a write and before commit
        set<int> sites_accessed;
        for(operation& op : t.operations) {
            if(op.op_type == 'W') {
                for(int site : SiteManager::getInstance()->site_var_map[op.variable_id]) {
                    sites_accessed.insert(site);
                }
            }
        }

        for(int site : sites_accessed) {
            if(!SiteManager::getInstance()->checksite(site)) {
                cout << "site down after write " << site << "transaction " << transaction_id << endl;
                return false;
            }
        }

        // first-committer-wins for write-write conflicts
        /*for(const operation& op : t.operations) {
            if(op.op_type == 'W') {
                for(const auto& log : DataManager::getInstance()->commit_logs) {
                    if(log.op_type == 'W' && log.variable_id == op.variable_id && 
                       log.timestamp > t.start_timestamp && log.timestamp < timestamp) {
                        cout << "Aborted " << transaction_id << endl;
                        return false;
                    }
                }
            }
        }*/

        //First commit rule
        for(auto itr = committed_nodes.begin(); itr!=committed_nodes.end(); itr++){
            Transaction* tran = &transactions_data[(*itr)]; 
            if(tran->commit_timestamp >= trans->start_timestamp){
                for(int var_id = 0;var_id<20;var_id++){
                    if(trans->data_vals[var_id].first != INT_MAX && tran->data_vals[var_id].first != INT_MAX){
                        cout << "Aborted by first commit rule trans_id " << transaction_id << endl;
                        trans->aborted = true;
                        cout  << transaction_id << " aborted: 1st Commit Rule "<< (tran->transaction_id) << endl;
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void executeWrite(int timestamp_no, int transaction_id, int var_id, int val){
       
        Transaction* trans = &transactions_data[transaction_id];
       
        //Think of the problem when a transaction aborts
        if(trans->aborted) return;
        
        // We just roll back the SiteManager state to the previous commit in global storage
        
        DataManager::getInstance()->write_trans[var_id].insert(transaction_id);
        for(int i=0;i<SiteManager::getInstance()->site_var_map[var_id].size();i++){
            if(SiteManager::getInstance()->checksite(SiteManager::getInstance()->site_var_map[var_id][i])) break;
            else if(i == SiteManager::getInstance()->site_var_map[var_id].size()-1){
                // All sites are down
                operation oper;
                oper.op_type = 'W';
                oper.transaction_id = transaction_id;
                oper.val = val;
                oper.variable_id = var_id;
                oper.timestamp = timestamp_no;
                wait_operations.push_back(oper);
                return;
            }
        }
        for(int i=0;i<SiteManager::getInstance()->site_var_map[var_id].size();i++){
            if(SiteManager::getInstance()->checksite(SiteManager::getInstance()->site_var_map[var_id][i])) trans->data_vals[var_id].second.push_back(SiteManager::getInstance()->site_var_map[var_id][i]);
        }
        trans->data_vals[var_id].first = val; 
        cout << "transaction " << transaction_id <<" variable " << var_id + 1 << " value " << val << " written " << endl;

        //int site = SiteManager::getsite(var_id);
        // Now update all accessed sites
        SiteManager::getInstance()->update_all_sites(var_id,val, transaction_id);

        /*int site = SiteManager::getInstance()->getsite(var_id, transaction_id);
        if(site == -1 || site == INT_MAX) return;*/
        //DataManager::writeval(timestamp,site, var_id, val);
        // we update global data by calling datamanager at commit time.
    }

    void executeFailSite(int op_timestamp,int site){
        SiteManager::getInstance()->failsite(op_timestamp,site);
    }

    void executeEnd(int timestamp_no,int transaction_id){
        cout << "end " << transaction_id <<" at "<<timestamp_no << endl;
        Transaction* trans = &transactions_data[transaction_id];
        /* 3 steps :- 
          0. Check abort value. if aborted, change sitemanager data into previous timestamp info
          1. copy sitemanager data into global dictionary copy
          2. write all transaction operations into commit logs */
        if(trans->aborted) {cout << "transaction aborted transaction- " << transaction_id << endl;return;}
        if(!can_commit(transaction_id)) {cout << "can_commit fail for "<<transaction_id << endl;return;}
        /*if(transactions[transaction_id].aborted){
            //SiteManager::getInstance()->site_data = DataManager::getInstance()->main_data[timestamp_no-1];
            return;
        }*/

        // uPDATE rw graph
        // R(curr trans) -> W(edge)
        for(auto var_id:trans->read_vars){
            set<int> var_id_write_trans = DataManager::getInstance()->write_trans[var_id];
            for(auto itr = var_id_write_trans.begin();itr!=var_id_write_trans.end();itr++){
                if(committed_nodes.find(*itr)!=committed_nodes.end()) {
                    rw_graph[transaction_id].insert(*itr);
                    cout<<"Edge from "<<transaction_id<<" to "<<(*itr)<<endl;
                }
            }
        }

        // W(curr trans) <- R(edge)
        for(int i=0;i<20;i++){
            if(trans->data_vals[i].first != INT_MAX){
                set<int> var_id_read_trans = DataManager::getInstance()->read_trans[i];
                for(auto itr = var_id_read_trans.begin();itr!=var_id_read_trans.end();itr++){
                    if(committed_nodes.find(*itr)!=committed_nodes.end()) {
                        rw_graph[*itr].insert(transaction_id);
                        cout<<"Edge from "<<(*itr)<<" to "<<transaction_id<<endl;
                    }
                }
                
                // W -> W(edge)
                set<int> var_id_write_trans = DataManager::getInstance()->write_trans[i];
                for(auto itr = var_id_write_trans.begin();itr!=var_id_write_trans.end();itr++){
                    if(trans->start_timestamp < transactions_data[(*itr)].start_timestamp){
                        if(committed_nodes.find(*itr)!=committed_nodes.end()) {
                            rw_graph[transaction_id].insert((*itr));
                            cout<<"Edge from "<<transaction_id<<" to "<<(*itr)<<endl;
                        }
                    }
                    else{
                        if(committed_nodes.find(*itr)!=committed_nodes.end()) {
                            rw_graph[(*itr)].insert(transaction_id);
                            cout<<"Edge from "<<(*itr)<<" to "<<transaction_id<<endl;
                        }
                    }
                }
            }
        }
        if(dfs()){
            cout << "transaction "<< transaction_id << " aborted cycle detect" << endl;
            return;

        }

        // DataManager::getInstance()->last_commit.resize(20,pair<int,vector<int>> ());
        for(int i=0;i<20;i++){
            if(trans->data_vals[i].first != INT_MAX){
                DataManager::getInstance()->last_commit[i].first = timestamp_no;
                DataManager::getInstance()->last_commit[i].second = SiteManager::getInstance()->write_all_sites(transaction_id,i,trans->data_vals[i].first);
                cout<<"Variable "<<i+1<<" commit at time "<<timestamp_no<<endl;
                
            }
        }
        DataManager::getInstance()->all_commits.push_back(DataManager::getInstance()->last_commit);

        DataManager::getInstance()->main_data[timestamp_no] = SiteManager::getInstance()->site_data;
        DataManager::getInstance()->committed_timestamps.push_back(timestamp_no);
        DataManager::getInstance()->commit_logs.insert(end(DataManager::getInstance()->commit_logs),
                    transactions[transaction_id].operations.begin(),transactions[transaction_id].operations.end());
        for(int i=0;i<10;i++){
            if(!SiteManager::getInstance()->checksite(i)) continue;
            for(int j=0;j<20;j++){
                SiteManager::getInstance()->can_read[i][j] = true;
            }
        }  
        committed_nodes.insert(transaction_id);     
        trans->commit_timestamp = timestamp_no;     
        
        cout << "Commit Successfull Transaction "<< transaction_id  << endl;
    }

    void executeRecover(int site_id){
        /* 1. Copy data that is available from other sites
           2. set active status for site  */
           /*1. replicated :- */
           SiteManager::getInstance()->recover_site(site_id);
            for(int i=0;i<TransactionManager::getInstance()->wait_operations.size();i++){
                operation oper = TransactionManager::getInstance()->wait_operations[i];
                if(oper.wait_site_ids.find(site_id) != oper.wait_site_ids.end()){
                    TransactionManager::getInstance()->wait_operations.erase(TransactionManager::getInstance()->wait_operations.begin()+i);
                    if(oper.op_type == 'W') TransactionManager::getInstance()->executeWrite(oper.timestamp,oper.transaction_id,oper.variable_id,oper.val);
                    else if(oper.op_type == 'R') TransactionManager::getInstance()->executeRead(oper.timestamp,oper.transaction_id,oper.variable_id);
                    i--;
                }
            }
            for(int var_id : SiteManager::getInstance()->var_site_map[site_id]) {
                if(var_id % 2 == 1) {  // Only replicated variables
                    /*int source_site = getsite(var_id,0);
                    if(source_site != -1) {
                        site_data[site_id][var_id] = site_data[source_site][var_id];
                    }*/
                   // cout << site_id << " " << var_id <<  endl;
                    SiteManager::getInstance()->can_read[site_id][var_id] = false;   
                }
            }
    }
    

};

TransactionManager* TransactionManager::instance = nullptr;



operation parse_instruction(string line) {
    operation op;
    //cout << "started " << endl;
    op.timestamp = TransactionManager::getInstance()->timestamp++;
    
    regex begin_pattern("begin\\s*\\(\\s*T(\\d+)\\s*\\)");
    regex read_pattern("R\\s*\\(\\s*T(\\d+)\\s*,\\s*x(\\d+)\\s*\\)");
    regex write_pattern("W\\s*\\(\\s*T(\\d+)\\s*,\\s*x(\\d+)\\s*,\\s*(\\d+)\\s*\\)");
    regex end_pattern("end\\s*\\(\\s*T(\\d+)\\s*\\)");
    regex fail_pattern("fail\\s*\\(\\s*(\\d+)\\s*\\)");
    regex recover_pattern("recover\\s*\\(\\s*(\\d+)\\s*\\)");
    
    smatch matches;
    
    if(regex_match(line, matches, begin_pattern)) {
        op.op_type = 'B';
        op.transaction_id = stoi(matches[1]);
    }
    else if(regex_match(line, matches, read_pattern)) {
        op.op_type = 'R';
        op.transaction_id = stoi(matches[1]);
        op.variable_id = stoi(matches[2])-1;
    }
    else if(regex_match(line, matches, write_pattern)) {
        op.op_type = 'W';
        op.transaction_id = stoi(matches[1]);
        op.variable_id = stoi(matches[2])-1;
        op.val = stoi(matches[3]);
    }
    else if(regex_match(line, matches, end_pattern)) {
        op.op_type = 'E';
        op.transaction_id = stoi(matches[1]);
    }
    else if(regex_match(line, matches, fail_pattern)) {
        op.op_type = 'F';
        op.site_id = stoi(matches[1]) - 1;  // Convert to 0-based indexing
    }
    else if(regex_match(line, matches, recover_pattern)) {
        op.op_type = 'r';
        op.site_id = stoi(matches[1]) - 1;  // Convert to 0-based indexing
    }
    else if(line == "dump()") {
        op.op_type = 'D';
    }
    // cout << op.op_type << " " << op.val << " " << op.transaction_id << endl;
    return op;
}


int main(int argc, char* argv[]) {
   if (argc != 2) {
       cout << "Usage: " << argv[0] << " <input_file>" << endl;
       return 1;
   }

   ifstream inFile(argv[1]);
   if (!inFile) {
       cout << "Error opening file: " << argv[1] << endl;
       return 1;
   }

   string line;
   while (getline(inFile, line)) {
       line = regex_replace(line, regex("^\\s+|\\s+$"), "");
       if (line.empty() || line[0] == '/') continue;
       
       operation op = parse_instruction(line);
       TransactionManager::getInstance()->all_operations.push_back(op);
   }

   TransactionManager::getInstance()->executeInstructions();
  dump();
   return 0;
}