// Lab 2
// Potiekhina Valeriia K-27
// Variant 8 (replace_if)
// Defaul compiler of VS Community 2022

#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>
#include <random>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace std;
using namespace chrono;

vector<int> generate_data(size_t n) {
    mt19937 gen(random_device{}());
    uniform_int_distribution<int> dist(0, 100);
    vector<int> v(n);
    for (auto &x : v) x = dist(gen);
    return v;
}

template<typename Predicate>
double measure_replace_no_policy(const vector<int>& input, vector<int>& output, Predicate pred, int new_value) {
    auto start = high_resolution_clock::now();
    replace_if(output.begin(), output.end(), pred, new_value);
    auto end = high_resolution_clock::now();
    return duration<double, milli>(end - start).count();
}

template<typename Policy, typename Predicate>
double measure_replace_with_policy(Policy&& policy, vector<int>& output, Predicate pred, int new_value) {
    auto start = high_resolution_clock::now();
    replace_if(policy, output.begin(), output.end(), pred, new_value);
    auto end = high_resolution_clock::now();
    return duration<double, milli>(end - start).count();
}

template<typename Predicate>
double custom_parallel_replace(vector<int>& data, Predicate pred, int new_value, size_t K) {
    size_t N = data.size();
    vector<thread> threads;
    threads.reserve(K);
    size_t chunk = (N + K - 1) / K;

    auto start_time = high_resolution_clock::now();

    for (size_t i = 0; i < K; ++i) {
        size_t start = i * chunk;
        size_t end = min(start + chunk, N);
        if (start >= N) break;

        threads.emplace_back([&, start, end]() {
            replace_if(data.begin() + start, data.begin() + end, pred, new_value);
        });
    }

    for (auto& t : threads) t.join();

    auto end_time = high_resolution_clock::now();
    return duration<double, milli>(end_time - start_time).count();
}

template<typename Predicate>
void print_parallel_results(const vector<int>& data, vector<int>& output, Predicate pred, int new_value, const vector<size_t>& threads_count) {
    unsigned int hw_threads = thread::hardware_concurrency();
    cout << "\n--- Custom parallel replace_if ---\n";
    cout << "Hardware threads: " << hw_threads << endl;
    cout << "| K (threads) | time (ms) |\n";
    cout << "|------------|-----------|\n";

    double best_time = 0.0;
    size_t best_K = 0;
    bool first = true;

    for (auto K : threads_count) {
        auto temp = data;
        double t = custom_parallel_replace(temp, pred, new_value, K);
        cout << "| " << setw(10) << K << " | " << setw(9) << t << " |\n";
        if (first || t < best_time) { best_time = t; best_K = K; first = false; }
    }

    cout << "Best speed: K = " << best_K << " (" << best_time << " ms)\n";
    cout << "K/hardware_threads ratio: " << best_K << "/" << hw_threads << " = " << double(best_K)/hw_threads << "\n";
    cout << "* Time usually grows if K >> hardware threads due to thread overhead.\n";
}

int main() {
    vector<size_t> sizes = { 1000000, 5000000 };
    vector<size_t> threads_count = {1, 2, 4, 8, 16}; 

    auto pred = [](int x){ return x > 50; };
    int new_value = 50;

    for (auto n : sizes) {
        cout << "\n=== Data size: " << n << " ===\n";
        vector<int> data = generate_data(n);

        vector<int> out1 = data;
        double t1 = measure_replace_no_policy(data, out1, pred, new_value);
        cout << "1) Sequential replace_if: " << t1 << " ms\n";

        vector<int> out2 = data;
        double t_seq = measure_replace_with_policy(execution::seq, out2, pred, new_value);
        vector<int> out3 = data;
        double t_par = measure_replace_with_policy(execution::par, out3, pred, new_value);
        vector<int> out4 = data;
        double t_par_unseq = measure_replace_with_policy(execution::par_unseq, out4, pred, new_value);

        cout << "2a) Policy seq:       " << t_seq << " ms\n";
        cout << "2b) Policy par:       " << t_par << " ms\n";
        cout << "2c) Policy par_unseq: " << t_par_unseq << " ms\n";

        print_parallel_results(data, out1, pred, new_value, threads_count);
    }

    return 0;
}
