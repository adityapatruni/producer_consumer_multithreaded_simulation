#include <iostream>
#include <fstream>
#include <random>
#include <time.h>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <queue>

using namespace std;
using namespace std::literals::chrono_literals;

#define MAXPART_TIMEOUT 3000
#define MAXPROD_TIMEOUT 6000
#define NUM_ITER 5

class Manufacturing_Plant {
private:
    mutex m1;
    condition_variable ProdCV, PartCV;
    vector<int> buffer;
    vector<int> loadOrder;
    vector<int> pickupOrder;
    vector<int> cart;
    int completed_prods;
    int wait_part, wait_product;
    int randomSeed;
    mutex mload;
    mutex mpickup;
    mutex mfile;
    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    condition_variable buffer_available;
    bool buffer_not_in_use = false;
    mutex mbuffer;
    bool is_buffer_open = true;
    ofstream myfile;
    bool prev_load_order;
    bool prev_pickup_order;

public:
    bool check_buffer_load();
    bool check_buffer_pickup();
    void PaWorker(int id);
    void ProdWorker(int id);
};
bool Manufacturing_Plant::check_buffer_load() {
    queue<int> index_queue;
    for (int i = 0; i < loadOrder.size(); i++) {
        if (loadOrder[i] > 0) index_queue.emplace(i);
    }
    while (!index_queue.empty()) {
        int curr_index = index_queue.front();
        index_queue.pop();
        if (curr_index == 0 && (5 - buffer[0] > 0)) {
            return true;
        }
        else if (curr_index == 1 && (5 - buffer[1] > 0)) {
            return true;
        }
        else if (curr_index == 2 && (4 - buffer[2] > 0)) {
            return true;
        }
        else if (curr_index == 3 && (3 - buffer[3] > 0)) {
            return true;
        }
        else if (curr_index == 4 && (3 - buffer[4] > 0)) {
            return true;
        }
    }
    return false;
}

bool Manufacturing_Plant::check_buffer_pickup() {
    queue<int> index_queue;
    for (int i = 0; i < pickupOrder.size(); i++) {
        if (pickupOrder[i] > 0) index_queue.emplace(i);
    }
    while (!index_queue.empty()) {
        int curr_index = index_queue.front();
        index_queue.pop();
        if (curr_index == 0 && (buffer[0] > 0)) {
            return true;
        }
        else if (curr_index == 1 && (buffer[1] > 0)) {
            return true;
        }
        else if (curr_index == 2 && (buffer[2] > 0)) {
            return true;
        }
        else if (curr_index == 3 && (buffer[3] > 0)) {
            return true;
        }
        else if (curr_index == 4 && (buffer[4] > 0)) {
            return true;
        }
    }
    return false;
}

void Manufacturing_Plant::PaWorker(int id)
{

    for (int i = 0; i < NUM_ITER; i++) {
        randomSeed++;
        unique_lock<mutex> ul(mload);
        while (!check_buffer_load()) { PartCV.wait(ul); }
        int check_sum = 0;
        vector<int> indexes;
        for (int i = 0; i < loadOrder.size(); i++) {
            if (loadOrder[i] > 0) {
                prev_load_order = true;
                check_sum += loadOrder[i];
                indexes.push_back(i);
            }
        }
        if (!prev_load_order) {
            indexes.push_back(rand() % 5);
            indexes.push_back(rand() % 5);
            indexes.push_back(rand() % 5);
        }
        bool is_load_complete;
        //auto nowTime = std::chrono::system_clock::now();
        //chrono::system_clock::duration RunTime = startTime - nowTime;
        std::srand(randomSeed);
        thread::id this_id = this_thread::get_id();
        // To make the sum of the final list as n

        for (int k = 0; k < 5 - check_sum; k++) {
            loadOrder[rand() % 5]++;
        }
        int sleepTime = 50 * loadOrder[0] + 50 * loadOrder[1] + 60 * loadOrder[2] + 60 * loadOrder[3] + 70 * loadOrder[4];
        this_thread::sleep_for(chrono::microseconds(sleepTime)); //time taken to make all parts
        int moveTime = 20 * loadOrder[0] + 20 * loadOrder[1] + 30 * loadOrder[2] + 30 * loadOrder[3] + 40 * loadOrder[4];
        this_thread::sleep_for(chrono::microseconds(moveTime)); //time taken to move to buffer
        //push_to_buffer(id);
        unique_lock<mutex> ulock(mbuffer);
        auto waitStartTime = std::chrono::system_clock::now();
        int waitTime = MAXPART_TIMEOUT;
        int waits = 0;
    waitHere:
        wait_part++;
        if (PartCV.wait_for(ulock, chrono::microseconds(waitTime), [this, &waits] {
            if (!check_buffer_load()) waits++;
            return check_buffer_load();
            })) {

            wait_part--;
            auto waitEndTime = std::chrono::system_clock::now();
            auto total_wait_time = waitEndTime - waitStartTime;
            myfile.open("log.txt", std::ofstream::out | std::ofstream::app);
            auto nowTime = std::chrono::system_clock::now();
            auto time_elapsed = nowTime - startTime;
            myfile << "Current time:" << chrono::duration_cast<chrono::microseconds>(time_elapsed).count() << "us" << endl;
            myfile << "Iteration " << i + 1 << endl;
            myfile << "Part Worker ID: " << id /*this_thread::get_id()*/ << endl;
            if (waits != 0) myfile << "Status: Wakeup-Notified" << endl;
            else myfile << "Status: New Load Order" << endl;
            myfile << "Accumulated Wait Time: " << chrono::duration_cast<chrono::microseconds>(total_wait_time).count() << "us" << endl;
            myfile << "Buffer State: (";
            for (int i = 0; i < buffer.size(); i++) {
                if (i == buffer.size() - 1) myfile << buffer[i];
                else {
                    myfile << buffer[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Load Order: (";
            for (int i = 0; i < loadOrder.size(); i++) {
                if (i == loadOrder.size() - 1) myfile << loadOrder[i];
                else {
                    myfile << loadOrder[i] << ",";
                }
            }
            myfile << ")" << endl;

            int p1 = loadOrder[0], p2 = loadOrder[1], p3 = loadOrder[2], p4 = loadOrder[3], p5 = loadOrder[4];
            for (int i = 0; i < p1; i++) {
                if (buffer[0] == 5) {
                    //is_buffer_open = false;
                    break;
                }
                buffer[0]++;
                loadOrder[0]--;

            }
            for (int i = 0; i < p2; i++) {
                if (buffer[1] == 5) {
                    //is_buffer_open = false;
                    break;
                }
                buffer[1]++;
                loadOrder[1]--;

            }
            for (int i = 0; i < p3; i++) {
                if (buffer[2] == 4) {
                    //is_buffer_open = false;
                    break;
                }
                buffer[2]++;
                loadOrder[2]--;

            }
            for (int i = 0; i < p4; i++) {
                if (buffer[3] == 3) {
                    //is_buffer_open = false;
                    break;
                }
                buffer[3]++;
                loadOrder[3]--;

            }
            for (int i = 0; i < p5; i++) {
                if (buffer[4] == 3) {
                    //is_buffer_open = false;
                    break;
                }
                buffer[4]++;
                loadOrder[4]--;

            }

            myfile << "Updated Buffer State: (";
            for (int i = 0; i < buffer.size(); i++) {
                if (i == buffer.size() - 1) myfile << buffer[i];
                else {
                    myfile << buffer[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Updated Load Order: (";
            for (int i = 0; i < loadOrder.size(); i++) {
                if (i == loadOrder.size() - 1) myfile << loadOrder[i];
                else {
                    myfile << loadOrder[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << endl;
            myfile.close();

            for (int i = 0; i < loadOrder.size(); i++) {
                if (loadOrder[i] > 0) {
                    is_load_complete = false;
                    break;
                }
                is_load_complete = true;
            }
            if (!is_load_complete) {
                ProdCV.notify_one();
                goto waitHere;
            }
        }
        else {
            wait_part--;
            auto waitEndTime = std::chrono::system_clock::now();
            auto total_wait_time = waitEndTime - waitStartTime;
            myfile.open("log.txt", std::ofstream::out | std::ofstream::app);
            auto nowTime = std::chrono::system_clock::now();
            auto time_elapsed = nowTime - startTime;
            myfile << "Current time:" << chrono::duration_cast<chrono::microseconds>(time_elapsed).count() << "us" << endl;
            myfile << "Iteration " << i + 1 << endl;
            myfile << "Part Worker ID: " << id /*this_thread::get_id()*/ << endl;
            myfile << "Status: Wakeup-Timeout" << endl;
            myfile << "Accumulated Wait Time: " << chrono::duration_cast<chrono::microseconds>(total_wait_time).count() << "us" << endl;
            myfile << "Buffer State: (";
            for (int i = 0; i < buffer.size(); i++) {
                if (i == buffer.size() - 1) myfile << buffer[i];
                else {
                    myfile << buffer[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Load Order: (";
            for (int i = 0; i < loadOrder.size(); i++) {
                if (i == loadOrder.size() - 1) myfile << loadOrder[i];
                else {
                    myfile << loadOrder[i] << ",";
                }
            }
            myfile << ")" << endl;

            if (check_buffer_load()) {
                int p1 = loadOrder[0], p2 = loadOrder[1], p3 = loadOrder[2], p4 = loadOrder[3], p5 = loadOrder[4];
                for (int i = 0; i < p1; i++) {
                    if (buffer[0] == 5) {
                        //is_buffer_open = false;
                        break;
                    }
                    buffer[0]++;
                    loadOrder[0]--;

                }
                for (int i = 0; i < p2; i++) {
                    if (buffer[1] == 5) {
                        //is_buffer_open = false;
                        break;
                    }
                    buffer[1]++;
                    loadOrder[1]--;

                }
                for (int i = 0; i < p3; i++) {
                    if (buffer[2] == 4) {
                        //is_buffer_open = false;
                        break;
                    }
                    buffer[2]++;
                    loadOrder[2]--;

                }
                for (int i = 0; i < p4; i++) {
                    if (buffer[3] == 3) {
                        //is_buffer_open = false;
                        break;
                    }
                    buffer[3]++;
                    loadOrder[3]--;

                }
                for (int i = 0; i < p5; i++) {
                    if (buffer[4] == 3) {
                        //is_buffer_open = false;
                        break;
                    }
                    buffer[4]++;
                    loadOrder[4]--;

                }
            }
            else {
                moveTime = 20 * loadOrder[0] + 20 * loadOrder[1] + 30 * loadOrder[2] + 30 * loadOrder[3] + 40 * loadOrder[4];
                this_thread::sleep_for(chrono::microseconds(moveTime));
            }
            myfile << "Updated Buffer State: (";
            for (int i = 0; i < buffer.size(); i++) {
                if (i == buffer.size() - 1) myfile << buffer[i];
                else {
                    myfile << buffer[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Updated Load Order: (";
            for (int i = 0; i < loadOrder.size(); i++) {
                if (i == loadOrder.size() - 1) myfile << loadOrder[i];
                else {
                    myfile << loadOrder[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << endl;
            myfile.close();
        }
        if (buffer[0] == 5 || buffer[1] == 5 || buffer[2] == 4 || buffer[3] == 3 || buffer[4] == 3) ProdCV.notify_all();
        else if (wait_part > wait_product) {
            PartCV.notify_one();
        }
        else {
            ProdCV.notify_one();
        }
    }
    if (wait_product > 0) ProdCV.notify_all();
    if (wait_part > 0) PartCV.notify_all();
}

void Manufacturing_Plant::ProdWorker(int id) //generate random pickup order
{
    //vector<int> v(5);
    for (int i = 0; i < NUM_ITER; i++) {
        randomSeed++;
        unique_lock<mutex> ul(mpickup);
        while (!check_buffer_pickup()) { ProdCV.wait(ul); }
        prev_pickup_order = false;
        vector<int> indexes;
        int check_sum = 0;
        for (int i = 0; i < pickupOrder.size(); i++) {
            if (pickupOrder[i] > 0) {
                prev_pickup_order = true;
                check_sum += pickupOrder[i];
                indexes.push_back(i);
            }
        }
        srand(randomSeed);
        if (!prev_pickup_order) {
            indexes.push_back(rand() % 5);
            indexes.push_back(rand() % 5);
            indexes.push_back(rand() % 5);
        }

        // To make the sum of elements 5
        srand(randomSeed);
        for (int i = 0; i < 5 - check_sum; i++) {
            pickupOrder[indexes[rand() % indexes.size()]]++;
        }
        indexes.clear();
        //pickup_from_buffer(id);
        unique_lock<mutex> ulock(mbuffer);
        vector<int> local(5);
        bool is_pickup_complete = true;
        int waitTime = MAXPROD_TIMEOUT;
        int waits = 0;
        auto waitStartTime = std::chrono::system_clock::now();
    waitHere:
        wait_product++;
        if (ProdCV.wait_for(ulock, chrono::microseconds(waitTime), [this, &waits] {
            if (!check_buffer_pickup()) waits++;
            return check_buffer_pickup();
            })) {
            wait_product--;
            auto waitEndTime = std::chrono::system_clock::now();
            auto total_wait_time = waitEndTime - waitStartTime;
            //unique_lock<mutex> ulockf(mfile);
            ofstream myfile;
            myfile.open("log.txt", std::ofstream::out | std::ofstream::app);
            auto nowTime = std::chrono::system_clock::now();
            auto time_elapsed = nowTime - startTime;
            myfile << "Current time:" << chrono::duration_cast<chrono::microseconds>(time_elapsed).count() << "us" << endl;
            myfile << "Iteration " << i + 1 << endl;
            myfile << "Product Worker ID: " << id /*this_thread::get_id()*/ << endl;
            if (waits != 0) myfile << "Status: Wakeup-Notified" << endl;
            else myfile << "Status: New Pickup Order" << endl;
            myfile << "Accumulated Wait Time: " << chrono::duration_cast<chrono::microseconds>(total_wait_time).count() << "us" << endl;
            myfile << "Buffer State: (";
            for (int i = 0; i < buffer.size(); i++) {
                if (i == buffer.size() - 1) myfile << buffer[i];
                else {
                    myfile << buffer[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Pickup Order: (";
            for (int i = 0; i < pickupOrder.size(); i++) {
                if (i == pickupOrder.size() - 1) myfile << pickupOrder[i];
                else {
                    myfile << pickupOrder[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Local State: (";
            for (int i = 0; i < local.size(); i++) {
                if (i == local.size() - 1) myfile << local[i];
                else {
                    myfile << local[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Cart State: (";
            for (int i = 0; i < cart.size(); i++) {
                if (i == cart.size() - 1) myfile << cart[i];
                else {
                    myfile << cart[i] << ",";
                }
            }
            myfile << ")" << endl;
            int assembly_Time = 0;
            int p1 = pickupOrder[0], p2 = pickupOrder[1], p3 = pickupOrder[2], p4 = pickupOrder[3], p5 = pickupOrder[4];
            for (int i = 0; i < p1; i++) {
                if (buffer[0] == 0) break;
                buffer[0]--;
                pickupOrder[0]--;
                cart[0]++;
                assembly_Time += 20;

            }
            for (int i = 0; i < p2; i++) {
                if (buffer[1] == 0) break;
                buffer[1]--;
                pickupOrder[1]--;
                cart[1]++;
                assembly_Time += 20;

            }
            for (int i = 0; i < p3; i++) {
                if (buffer[2] == 0) break;
                buffer[2]--;
                pickupOrder[2]--;
                cart[2]++;
                assembly_Time += 30;

            }
            for (int i = 0; i < p4; i++) {
                if (buffer[3] == 0) break;
                buffer[3]--;
                pickupOrder[3]--;
                cart[3]++;
                assembly_Time += 30;

            }
            for (int i = 0; i < p5; i++) {
                if (buffer[4] == 0) break;
                buffer[4]--;
                pickupOrder[4]--;
                cart[4]++;
                assembly_Time += 40;

            }
            myfile << "Updated Buffer State: (";
            for (int i = 0; i < buffer.size(); i++) {
                if (i == buffer.size() - 1) myfile << buffer[i];
                else {
                    myfile << buffer[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Updated Pickup Order: (";
            for (int i = 0; i < pickupOrder.size(); i++) {
                if (i == pickupOrder.size() - 1) myfile << pickupOrder[i];
                else {
                    myfile << pickupOrder[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Updated Local State: (";
            for (int i = 0; i < local.size(); i++) {
                if (i == local.size() - 1) myfile << local[i];
                else {
                    myfile << local[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Updated Cart State: (";
            for (int i = 0; i < cart.size(); i++) {
                if (i == cart.size() - 1) myfile << cart[i];
                else {
                    myfile << cart[i] << ",";
                }
            }
            myfile << ")" << endl;
            for (int i = 0; i < pickupOrder.size(); i++) {
                if (pickupOrder[i] > 0) {
                    is_pickup_complete = false;
                    break;
                }
                is_pickup_complete = true;
            }
            if (!is_pickup_complete) {

                myfile << "Total Completed Products: " << completed_prods << endl;
                myfile << endl;
                myfile.close();
                PartCV.notify_all();
                goto waitHere;
            }
            else {
                this_thread::sleep_for(chrono::microseconds(assembly_Time));
                completed_prods++;
                myfile << "Total Completed Products: " << completed_prods << endl;
                myfile << endl;
                myfile.close();
            }
        }
        else { //timeout
            wait_product--;
            auto waitEndTime = std::chrono::system_clock::now();
            auto total_wait_time = waitEndTime - waitStartTime;
            //unique_lock<mutex> ulockf(mfile);
            ofstream myfile;
            myfile.open("log.txt", std::ofstream::out | std::ofstream::app);
            auto nowTime = std::chrono::system_clock::now();
            auto time_elapsed = nowTime - startTime;
            myfile << "Current time:" << chrono::duration_cast<chrono::microseconds>(time_elapsed).count() << "us" << endl;
            myfile << "Iteration " << i + 1 << endl;
            myfile << "Product Worker ID: " << id /*this_thread::get_id()*/ << endl;
            myfile << "Status: Wakeup-Timeout" << endl;
            myfile << "Accumulated Wait Time: " << chrono::duration_cast<chrono::microseconds>(total_wait_time).count() << "us" << endl;
            myfile << "Buffer State: (";
            for (int i = 0; i < buffer.size(); i++) {
                if (i == buffer.size() - 1) myfile << buffer[i];
                else {
                    myfile << buffer[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Pickup Order: (";
            for (int i = 0; i < pickupOrder.size(); i++) {
                if (i == pickupOrder.size() - 1) myfile << pickupOrder[i];
                else {
                    myfile << pickupOrder[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Local State: (";
            for (int i = 0; i < pickupOrder.size(); i++) {
                if (i == pickupOrder.size() - 1) myfile << 0;
                else {
                    myfile << 0 << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Cart State: (";
            for (int i = 0; i < cart.size(); i++) {
                if (i == cart.size() - 1) myfile << cart[i];
                else {
                    myfile << cart[i] << ",";
                }
            }
            myfile << ")" << endl;
            int assembly_Time = 0;
            int p1 = pickupOrder[0], p2 = pickupOrder[1], p3 = pickupOrder[2], p4 = pickupOrder[3], p5 = pickupOrder[4];
            for (int i = 0; i < p1; i++) {
                if (buffer[0] == 0) break;
                buffer[0]--;
                pickupOrder[0]--;
                cart[0]++;
                assembly_Time += 20;

            }
            for (int i = 0; i < p2; i++) {
                if (buffer[1] == 0) break;
                buffer[1]--;
                pickupOrder[1]--;
                cart[1]++;
                assembly_Time += 20;

            }
            for (int i = 0; i < p3; i++) {
                if (buffer[2] == 0) break;
                buffer[2]--;
                pickupOrder[2]--;
                cart[2]++;
                assembly_Time += 30;

            }
            for (int i = 0; i < p4; i++) {
                if (buffer[3] == 0) break;
                buffer[3]--;
                pickupOrder[3]--;
                cart[3]++;
                assembly_Time += 30;

            }
            for (int i = 0; i < p5; i++) {
                if (buffer[4] == 0) break;
                buffer[4]--;
                pickupOrder[4]--;
                cart[4]++;
                assembly_Time += 40;

            }
            for (int i = 0; i < pickupOrder.size(); i++) {
                if (pickupOrder[i] > 0) {
                    is_pickup_complete = false;
                    break;
                }
                is_pickup_complete = true;
            }
            if (!is_pickup_complete) {
                local = pickupOrder;
                int moveTime = cart[0] * 20 + cart[1] * 20 + cart[2] * 30 + cart[3] * 30 + cart[4] * 40;
                this_thread::sleep_for(chrono::microseconds(moveTime));
                cart = { 0, 0, 0, 0, 0 };
            }
            else {
                this_thread::sleep_for(chrono::microseconds(assembly_Time));
                completed_prods++;
            }
            myfile << "Updated Buffer State: (";
            for (int i = 0; i < buffer.size(); i++) {
                if (i == buffer.size() - 1) myfile << buffer[i];
                else {
                    myfile << buffer[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Updated Pickup Order: (";
            for (int i = 0; i < pickupOrder.size(); i++) {
                if (i == pickupOrder.size() - 1) myfile << pickupOrder[i];
                else {
                    myfile << pickupOrder[i] << ",";
                }
            }
            myfile << ")" << endl;
            local = pickupOrder;
            myfile << "Updated Local State: (";
            for (int i = 0; i < local.size(); i++) {
                if (i == local.size() - 1) myfile << local[i];
                else {
                    myfile << local[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Updated Cart State: (";
            for (int i = 0; i < cart.size(); i++) {
                if (i == cart.size() - 1) myfile << cart[i];
                else {
                    myfile << cart[i] << ",";
                }
            }
            myfile << ")" << endl;
            myfile << "Total Completed Products: " << completed_prods << endl;
            myfile << endl;
            myfile.close();
        }
        if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 0 && buffer[4] == 0) PartCV.notify_all();
        else if (wait_product > wait_part) ProdCV.notify_one();
        else PartCV.notify_all();
    }
    if (wait_part > 0) PartCV.notify_all();
    if (wait_product > 0) ProdCV.notify_all();
}

void PartWorker(Manufacturing_Plant &MF, int id) {
    MF.PaWorker(id);
}

void ProductWorker(Manufacturing_Plant& MF, int id) {
    MF.ProdWorker(id);
}



// Driver code
int main() {
    const int m = 20, n = 16; //m: number of Part Workers
    //n: number of Product Workers
    //m>n

    Manufacturing_Plant MP;
    thread partW[m];
    thread prodW[n];
    for (int i = 0; i < n; i++) {
        partW[i] = thread(PartWorker, ref(MP), i);
        prodW[i] = thread(ProductWorker, ref(MP), i);
    }
    for (int i = n; i < m; i++) {
        partW[i] = thread(PartWorker, ref(MP), i);
    }
    /* Join the threads to the main threads */
    for (int i = 0; i < n; i++) {
        partW[i].join();
        prodW[i].join();
    }
    for (int i = n; i < m; i++) {
        partW[i].join();
    }

    cout << "Finish!" << endl;
    return 0;
}
