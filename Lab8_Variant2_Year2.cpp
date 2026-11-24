#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

// Двоичный поиск в отсортированном массиве. При отсутствии ключа возвращаем -1.
int binarySearchRecursive(const std::vector<int>& a, int left, int right, int key) {
    if (left > right) return -1;
    const int mid = left + (right - left) / 2;
    if (a[mid] == key) return mid;
    if (a[mid] < key) return binarySearchRecursive(a, mid + 1, right, key);
    return binarySearchRecursive(a, left, mid - 1, key);
}

int binarySearchRecursive(const std::vector<int>& a, int key) {
    return binarySearchRecursive(a, 0, static_cast<int>(a.size()) - 1, key);
}

int binarySearchIterative(const std::vector<int>& a, int key) {
    int left = 0, right = static_cast<int>(a.size()) - 1;
    while (left <= right) {
        const int mid = left + (right - left) / 2;
        if (a[mid] == key) return mid;
        if (a[mid] < key) left = mid + 1;
        else right = mid - 1;
    }
    return -1;
}

// Хеш-таблица фиксированной ёмкости с открытой адресацией.
class HashTable {
    enum class State { Empty, Occupied, Deleted };
    struct Entry {
        int key = 0;
        int value = 0;
        State state = State::Empty;
    };
    std::vector<Entry> table;
    std::size_t count = 0;

    static std::size_t checkedCapacity(std::size_t capacity) {
        if (capacity == 0) throw std::invalid_argument("Zero capacity");
        return capacity;
    }

    // Перемешивание битов целого ключа, включая отрицательные значения.
    std::size_t hash(int key) const {
        std::uint32_t x = static_cast<std::uint32_t>(key);
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return static_cast<std::size_t>(x) % table.size();
    }

    std::size_t next(std::size_t pos) const {
        return pos + 1 == table.size() ? 0 : pos + 1;
    }

    std::size_t locate(int key) const {
        std::size_t pos = hash(key);
        for (std::size_t i = 0; i < table.size(); ++i, pos = next(pos)) {
            const Entry& e = table[pos];
            if (e.state == State::Empty) break;
            if (e.state == State::Occupied && e.key == key) return pos;
        }
        return table.size();
    }

    void put(std::size_t pos, int key, int value) {
        table[pos] = {key, value, State::Occupied};
        ++count;
    }

public:
    explicit HashTable(std::size_t capacity) : table(checkedCapacity(capacity)) {}
    std::size_t size() const { return count; }
    std::size_t capacity() const { return table.size(); }
    bool empty() const { return count == 0; }
    double loadFactor() const { return static_cast<double>(count) / table.size(); }
    std::size_t bucketIndex(int key) const { return hash(key); }

    // Сначала ищем существующий ключ, даже если встретилась удалённая ячейка.
    // При заполненной таблице вставка нового ключа возвращает false.
    bool insert(int key, int value) {
        std::size_t pos = hash(key), deleted = table.size();
        for (std::size_t i = 0; i < table.size(); ++i, pos = next(pos)) {
            Entry& e = table[pos];
            if (e.state == State::Occupied && e.key == key) {
                e.value = value;
                return true;
            }
            if (e.state == State::Deleted && deleted == table.size()) deleted = pos;
            if (e.state == State::Empty) {
                put(deleted == table.size() ? pos : deleted, key, value);
                return true;
            }
        }
        if (deleted != table.size()) {
            put(deleted, key, value);
            return true;
        }
        return false;
    }

    bool find(int key, int& value) const {
        const std::size_t pos = locate(key);
        if (pos == table.size()) return false;
        value = table[pos].value;
        return true;
    }

    bool remove(int key) {
        const std::size_t pos = locate(key);
        if (pos == table.size()) return false;
        // Метка Deleted сохраняет цепочку пробирования.
        table[pos].state = State::Deleted;
        --count;
        return true;
    }

    void print() const {
        for (std::size_t i = 0; i < table.size(); ++i) {
            const Entry& e = table[i];
            std::cout << i << ": ";
            if (e.state == State::Occupied) std::cout << e.key << " -> " << e.value;
            else if (e.state == State::Deleted) std::cout << "DELETED";
            else std::cout << "EMPTY";
            std::cout << '\n';
        }
    }
};

int passed = 0;
void check(bool ok, const char* name) {
    std::cout << name << ": " << (ok ? "PASS" : "FAIL") << '\n';
    if (!ok) throw std::runtime_error(name);
    ++passed;
}

void testSearch() {
    std::cout << "\n=== PART 1: BINARY SEARCH ===\n";
    const std::vector<int> a{-10, -3, 0, 5, 8, 13, 21};
    auto both = [](const std::vector<int>& v, int key, int expected) {
        return binarySearchRecursive(v, key) == expected &&
               binarySearchIterative(v, key) == expected;
    };
    check(both(a, 5, 3), "Middle element");
    check(both(a, -10, 0), "First element");
    check(both(a, 21, 6), "Last element");
    check(both(a, -3, 1), "Negative element");
    check(both(a, 7, -1), "Missing inside range");
    check(both(a, -100, -1) && both(a, 100, -1), "Missing outside range");
    check(both({}, 1, -1), "Empty array");
    check(both({4}, 4, 0) && both({4}, 2, -1), "Singleton");
    const std::vector<int> duplicates{1, 2, 2, 2, 5};
    int r = binarySearchRecursive(duplicates, 2);
    int i = binarySearchIterative(duplicates, 2);
    check(r >= 1 && r <= 3 && i >= 1 && i <= 3, "Duplicates");
    check(both({std::numeric_limits<int>::min(), 0,
                std::numeric_limits<int>::max()}, std::numeric_limits<int>::max(), 2),
          "Integer limits");
    std::cout << "Search for 5: index " << binarySearchRecursive(a, 5) << '\n';
}

void testHash() {
    std::cout << "\n=== PART 2: LINEAR PROBING HASH TABLE ===\n";
    HashTable h(7);
    auto has = [](const HashTable& t, int key, int expected) {
        int value = 0;
        return t.find(key, value) && value == expected;
    };
    int value = 0;
    check(h.empty() && h.size() == 0 && h.capacity() == 7, "Initially empty");
    check(!h.find(10, value) && !h.remove(10), "Missing key");
    // Подбираем ключи, попадающие в последний бакет, для проверки перехода через край.
    std::vector<int> keys;
    for (int k = 0; keys.size() < 4; ++k)
        if (h.bucketIndex(k) == 6) keys.push_back(k);
    int a = keys[0], b = keys[1], c = keys[2], d = keys[3];
    std::cout << "Collision keys: " << a << ' ' << b << ' ' << c << '\n';
    check(h.insert(a, 10) && h.insert(b, 20) && h.insert(c, 30), "Insert collisions");
    check(has(h, a, 10) && has(h, b, 20) && has(h, c, 30), "Find after wraparound");
    check(h.size() == 3, "Size after insertion");
    check(h.remove(b) && !h.find(b, value), "Remove collision key");
    check(has(h, c, 30), "Search across deleted cell");
    check(h.insert(c, 99) && has(h, c, 99) && h.size() == 2, "Update across tombstone");
    check(h.insert(d, 40) && has(h, d, 40) && h.size() == 3, "Reuse deleted cell");
    check(!h.remove(b), "Repeated removal");
    std::cout << "Table after deletion and reuse:\n";
    h.print();
    check(h.loadFactor() > 0.42 && h.loadFactor() < 0.44, "Load factor");
    check(h.insert(-5, -100) && has(h, -5, -100), "Negative key and value");
    check(h.insert(std::numeric_limits<int>::min(), 42) &&
          has(h, std::numeric_limits<int>::min(), 42), "Minimum integer key");
    check(h.insert(std::numeric_limits<int>::max(), 0) &&
          has(h, std::numeric_limits<int>::max(), 0), "Maximum key and zero value");
    check(h.insert(1000000, 8) && h.size() == h.capacity(), "Fill table");
    check(!h.insert(1000001, 9) && h.size() == 7, "Reject full table insertion");
    check(!h.find(1000001, value) && !h.remove(1000001), "Full table missing key");
    check(h.insert(a, -1) && has(h, a, -1) && h.size() == 7, "Update full table");
    check(h.remove(a) && h.insert(1000001, 9) && has(h, 1000001, 9), "Reuse full table tombstone");
    HashTable one(1);
    check(one.insert(0, 0) && has(one, 0, 0) && !one.insert(1, 1), "Capacity one");
    check(one.remove(0) && one.empty() && !one.find(0, value) &&
          one.insert(1, 1) && has(one, 1, 1), "All cells deleted and reuse");
    bool rejected = false;
    try { HashTable invalid(0); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "Zero capacity rejected");
}

// Измеряем только поиск. Контрольная сумма не позволяет удалить вычисления.
struct Measurement { double time; std::int64_t checksum; };
template <typename Search>
Measurement benchmark(const std::vector<int>& queries, Search search) {
    using Clock = std::chrono::steady_clock;
    double times[3]{};
    std::int64_t checksum = 0;
    for (int trial = 0; trial < 3; ++trial) {
        std::int64_t sum = 0;
        auto start = Clock::now();
        for (int key : queries) sum += static_cast<std::int64_t>(search(key)) + 1;
        auto end = Clock::now();
        times[trial] = std::chrono::duration<double, std::micro>(end - start).count();
        if (trial && checksum != sum) throw std::runtime_error("Inconsistent benchmark");
        checksum = sum;
    }
    std::sort(times, times + 3);
    return {times[1], checksum};
}

void comparePerformance() {
    std::cout << "\n=== PART 3: PERFORMANCE COMPARISON ===\n";
    std::cout << "10000 identical queries; 50% hits; median of 3 runs (us)\n";
    std::cout << "Table construction and query generation excluded\n";
    std::cout << "Capacity = 2*N+1; load factor about 0.5\n";
    std::cout << std::setw(9) << "N" << std::setw(13) << "Iterative"
              << std::setw(13) << "Recursive" << std::setw(13) << "Hash"
              << std::setw(11) << "Load" << '\n';
    std::mt19937 rng(2026);
    bool valid = true;
    for (int n : {1000, 10000, 100000}) {
        std::vector<int> a(n);
        HashTable h(static_cast<std::size_t>(2 * n + 1));
        for (int i = 0; i < n; ++i) {
            a[i] = 2 * i;
            if (!h.insert(a[i], i)) throw std::runtime_error("Insertion failed");
        }
        std::vector<int> queries;
        queries.reserve(10000);
        std::uniform_int_distribution<int> dist(0, n - 1);
        for (int i = 0; i < 5000; ++i) {
            queries.push_back(2 * dist(rng));
            queries.push_back(2 * dist(rng) + 1);
        }
        std::shuffle(queries.begin(), queries.end(), rng);
        auto iter = [&](int key) { return binarySearchIterative(a, key); };
        auto recur = [&](int key) { return binarySearchRecursive(a, key); };
        auto hash = [&](int key) { int v = -1; return h.find(key, v) ? v : -1; };
        std::int64_t expectedSum = 0;
        // Проверяем каждый результат отдельно, вне измеряемого участка.
        for (int key : queries) {
            const int expected = key % 2 == 0 ? key / 2 : -1;
            valid = valid && iter(key) == expected && recur(key) == expected && hash(key) == expected;
            expectedSum += static_cast<std::int64_t>(expected) + 1;
        }
        auto t1 = benchmark(queries, iter);
        auto t2 = benchmark(queries, recur);
        auto t3 = benchmark(queries, hash);
        valid = valid && t1.checksum == expectedSum && t2.checksum == expectedSum && t3.checksum == expectedSum;
        std::cout << std::fixed << std::setprecision(2) << std::setw(9) << n
                  << std::setw(13) << t1.time << std::setw(13) << t2.time
                  << std::setw(13) << t3.time << std::setw(11) << std::setprecision(4)
                  << h.loadFactor() << '\n';
    }
    check(valid, "Benchmark results verified");
}

int main() {
    try {
        std::cout << "LAB 8 - YEAR 2 - VARIANT 2\n";
        testSearch();
        testHash();
        comparePerformance();
        std::cout << "\n=== COMPLEXITY SUMMARY ===\n"
                  << "Binary search: best O(1), worst O(log n)\n"
                  << "Iterative memory: O(1); recursive stack: O(log n)\n"
                  << "Hash insert/find/remove: expected O(1), worst O(m)\n"
                  << "m = table capacity; memory O(m)\n"
                  << "Expected bound assumes good distribution and low probe occupancy\n"
                  << "\nTests passed: " << passed << '\n'
                  << "Program completed successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
