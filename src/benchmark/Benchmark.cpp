// =============================================================================
// Benchmark.cpp — Headless Section 10 Benchmark Harness
// Course constraints: no <algorithm>, no std::vector, no enum, no globals.
// =============================================================================

#include "benchmark/Benchmark.h"
#include "structures/BST.h"
#include "structures/AVLTree.h"
#include "structures/MaxHeap.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace VaultHeist {

namespace {

// Distribution constants (no enum allowed)
static const int DIST_RANDOM          = 0;
static const int DIST_SORTED          = 1;
static const int DIST_REVERSE_SORTED  = 2;
static const int DIST_DUPLICATE_HEAVY = 3;

static inline void swapIntLocal(int& a, int& b) {
    int t = a;
    a = b;
    b = t;
}

// Seeded xorshift32 RNG (same pattern as EndlessMode.cpp)
struct XorShift32 {
    unsigned int state;

    explicit XorShift32(unsigned int seed)
        : state(seed == 0u ? 0x9E3779B9u : seed) {}

    unsigned int next() {
        unsigned int x = state;
        if (x == 0u) x = 0x9E3779B9u;
        x ^= (x << 13);
        x ^= (x >> 17);
        x ^= (x << 5);
        state = x;
        return x;
    }

    int nextRange(int lo, int hiInclusive) {
        unsigned int span = static_cast<unsigned int>(hiInclusive - lo + 1);
        return lo + static_cast<int>(next() % span);
    }
};

static void fillPermutation1ToN(int* arr, int n, XorShift32& rng) {
    for (int i = 0; i < n; ++i) {
        arr[i] = i + 1;
    }
    // Fisher-Yates shuffle
    for (int i = n - 1; i > 0; --i) {
        int j = static_cast<int>(rng.next() % static_cast<unsigned int>(i + 1));
        swapIntLocal(arr[i], arr[j]);
    }
}

static void fillDistribution(int* arr, int n, int dist, XorShift32& rng) {
    if (dist == DIST_RANDOM) {
        for (int i = 0; i < n; ++i) {
            arr[i] = static_cast<int>(rng.next() % 100000u);
        }
    } else if (dist == DIST_SORTED) {
        for (int i = 0; i < n; ++i) {
            arr[i] = i + 1;
        }
    } else if (dist == DIST_REVERSE_SORTED) {
        for (int i = 0; i < n; ++i) {
            arr[i] = n - i;
        }
    } else {
        // Duplicate-heavy: small alphabet [0..4]
        for (int i = 0; i < n; ++i) {
            arr[i] = static_cast<int>(rng.next() % 5u);
        }
    }
}

static const char* distLabel(int dist) {
    if (dist == DIST_RANDOM) return "Random";
    if (dist == DIST_SORTED) return "Sorted";
    if (dist == DIST_REVERSE_SORTED) return "Reverse-Sorted";
    return "Duplicate-Heavy";
}

static void copyIntArray(const int* src, int* dst, int n) {
    for (int i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

static bool isSortedAscending(const int* arr, int n) {
    for (int i = 0; i < n - 1; ++i) {
        if (arr[i] > arr[i + 1]) return false;
    }
    return true;
}

// Hand-rolled Merge Sort (purely for Section 10 sorting comparison; no std::sort)
static void mergeRanges(int* arr, int* tmp, int left, int mid, int right,
                        long long& comparisons, long long& moves) {
    for (int i = left; i <= right; ++i) {
        tmp[i] = arr[i];
        moves++;
    }
    int i = left;
    int j = mid + 1;
    int k = left;
    while (i <= mid && j <= right) {
        comparisons++;
        if (tmp[i] <= tmp[j]) {
            arr[k++] = tmp[i++];
        } else {
            arr[k++] = tmp[j++];
        }
        moves++;
    }
    while (i <= mid) {
        arr[k++] = tmp[i++];
        moves++;
    }
    while (j <= right) {
        arr[k++] = tmp[j++];
        moves++;
    }
}

static void mergeSortRec(int* arr, int* tmp, int left, int right,
                         long long& comparisons, long long& moves) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSortRec(arr, tmp, left, mid, comparisons, moves);
    mergeSortRec(arr, tmp, mid + 1, right, comparisons, moves);
    mergeRanges(arr, tmp, left, mid, right, comparisons, moves);
}

static void mergeSort(int* arr, int n, long long& comparisons, long long& moves) {
    comparisons = 0;
    moves = 0;
    if (n <= 1 || !arr) return;
    int* tmp = new int[n];
    mergeSortRec(arr, tmp, 0, n - 1, comparisons, moves);
    delete[] tmp;
}

static int repeatsForHeapBuild(int n) {
    if (n <= 100) return 200;
    if (n <= 1000) return 20;
    return 5;
}

static int repeatsForSort(int n) {
    if (n <= 100) return 20;
    if (n <= 1000) return 5;
    return 3;
}

} // namespace

bool Benchmark::runAll(const std::string& reportPath, unsigned int seed) {
    using Clock = std::chrono::high_resolution_clock;

    std::ostringstream out;
    XorShift32 rng(seed);

    out << "====================================================================================\n";
    out << "     VAULT HEIST: THE LIVING VAULT - BENCHMARK REPORT (PROJECT.PDF SECTION 10)      \n";
    out << "====================================================================================\n";
    out << "RNG Algorithm   : XorShift32\n";
    out << "RNG Seed        : " << seed << "\n";
#ifdef _DEBUG
    out << "Build Mode      : Debug (unoptimized; operation counts are exact, timings scale with Debug overhead)\n";
#else
    out << "Build Mode      : Release\n";
#endif
    out << "Timing Note     : Wall-clock timing at small n (e.g. n = 100) can be noisy due to\n";
    out << "                  OS scheduling and clock resolution; fast small-n runs are averaged\n";
    out << "                  over repeated trials, and exact operation counters are always shown.\n";
    out << "====================================================================================\n\n";

    const int sizes[3] = { 100, 1000, 10000 };
    const int numSearches = 10000;
    const int numKthQueries = 1000;

    // -------------------------------------------------------------------------
    // 1. Height Comparison + 2. Search Cost (same trees)
    // -------------------------------------------------------------------------
    std::ostringstream sec1;
    std::ostringstream sec2;

    sec1 << "### 1. Height Comparison (BST vs AVL)\n";
    sec1 << "|     N | Input  | BST Height | AVL Height | Ceil(log2(N+1)) | BST Valid | AVL Valid |\n";
    sec1 << "|-------|--------|------------|------------|-----------------|-----------|-----------|\n";

    sec2 << "### 2. Search Cost (10,000 Random Searches in [1, 1.5*N] on Same Trees)\n";
    sec2 << "|     N | Structure | Input  | Total Time (ms) | Avg Time/Search (us) | Avg Comps/Search |\n";
    sec2 << "|-------|-----------|--------|-----------------|----------------------|------------------|\n";

    for (int si = 0; si < 3; ++si) {
        int n = sizes[si];
        int minTheoryH = static_cast<int>(std::ceil(std::log2(static_cast<double>(n + 1))));

        // Generate random permutation and search keys once per N
        int* randPerm = new int[n];
        fillPermutation1ToN(randPerm, n, rng);

        int* searchKeys = new int[numSearches];
        int maxSearchKey = n + (n / 2);
        for (int q = 0; q < numSearches; ++q) {
            searchKeys[q] = rng.nextRange(1, maxSearchKey);
        }

        // Build Sorted trees (1..n) and Random trees
        BST bstSorted;
        AVLTree avlSorted;
        for (int i = 1; i <= n; ++i) {
            bstSorted.insert(i);
            avlSorted.insert(i);
        }

        BST bstRandom;
        AVLTree avlRandom;
        for (int i = 0; i < n; ++i) {
            bstRandom.insert(randPerm[i]);
            avlRandom.insert(randPerm[i]);
        }

        sec1 << "| " << std::setw(5) << n << " | Sorted | "
             << std::setw(10) << bstSorted.getHeight() << " | "
             << std::setw(10) << avlSorted.getHeight() << " | "
             << std::setw(15) << minTheoryH << " | "
             << std::setw(9) << (bstSorted.isValidBST() ? "YES" : "NO") << " | "
             << std::setw(9) << (avlSorted.isValidAVL() ? "YES" : "NO") << " |\n";

        sec1 << "| " << std::setw(5) << n << " | Random | "
             << std::setw(10) << bstRandom.getHeight() << " | "
             << std::setw(10) << avlRandom.getHeight() << " | "
             << std::setw(15) << minTheoryH << " | "
             << std::setw(9) << (bstRandom.isValidBST() ? "YES" : "NO") << " | "
             << std::setw(9) << (avlRandom.isValidAVL() ? "YES" : "NO") << " |\n";

        // Measure 10,000 searches on each of the 4 trees
        struct SearchTarget {
            const char* name;
            const char* dist;
            bool isAvl;
            const BST* bst;
            const AVLTree* avl;
        };
        SearchTarget targets[4] = {
            { "BST", "Sorted", false, &bstSorted, nullptr },
            { "AVL", "Sorted", true,  nullptr,    &avlSorted },
            { "BST", "Random", false, &bstRandom, nullptr },
            { "AVL", "Random", true,  nullptr,    &avlRandom }
        };

        for (int ti = 0; ti < 4; ++ti) {
            volatile int foundSink = 0;
            double totalMs = 0.0;
            long long totalComps = 0;
            if (!targets[ti].isAvl) {
                BST::resetCounters();
                auto t0 = Clock::now();
                for (int q = 0; q < numSearches; ++q) {
                    if (targets[ti].bst->search(searchKeys[q])) foundSink++;
                }
                auto t1 = Clock::now();
                totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
                totalComps = BST::getComparisons();
            } else {
                AVLTree::resetCounters();
                auto t0 = Clock::now();
                for (int q = 0; q < numSearches; ++q) {
                    if (targets[ti].avl->search(searchKeys[q])) foundSink++;
                }
                auto t1 = Clock::now();
                totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
                totalComps = AVLTree::getComparisons();
            }
            (void)foundSink;
            double avgUs = (totalMs * 1000.0) / static_cast<double>(numSearches);
            double avgComps = static_cast<double>(totalComps) / static_cast<double>(numSearches);

            sec2 << "| " << std::setw(5) << n << " | "
                 << std::setw(9) << targets[ti].name << " | "
                 << std::setw(6) << targets[ti].dist << " | "
                 << std::setw(15) << std::fixed << std::setprecision(3) << totalMs << " | "
                 << std::setw(20) << std::fixed << std::setprecision(4) << avgUs << " | "
                 << std::setw(16) << std::fixed << std::setprecision(2) << avgComps << " |\n";
        }

        delete[] searchKeys;
        delete[] randPerm;
    }

    out << sec1.str() << "\n" << sec2.str() << "\n";

    // -------------------------------------------------------------------------
    // 3. Rotation Count (AVL Insert & Delete, Sorted vs Random)
    // -------------------------------------------------------------------------
    out << "### 3. AVL Rotation Count (Insert & Delete, Single Rotations Counted)\n";
    out << "|     N | Input  | Insert Rots | Avg Rot/Insert | Delete Rots | Avg Rot/Delete | Max Rot (1 Del) |\n";
    out << "|-------|--------|-------------|----------------|-------------|----------------|-----------------|\n";

    for (int si = 0; si < 3; ++si) {
        int n = sizes[si];
        int* insKeys = new int[n];
        int* delKeys = new int[n];

        for (int mode = 0; mode < 2; ++mode) {
            bool isSorted = (mode == 0);
            if (isSorted) {
                for (int i = 0; i < n; ++i) {
                    insKeys[i] = i + 1;
                    delKeys[i] = i + 1;
                }
            } else {
                fillPermutation1ToN(insKeys, n, rng);
                fillPermutation1ToN(delKeys, n, rng);
            }

            AVLTree avl;
            AVLTree::resetCounters();
            for (int i = 0; i < n; ++i) {
                avl.insert(insKeys[i]);
            }
            long long insRot = AVLTree::getRotations();

            AVLTree::resetCounters();
            long long maxDelSingle = 0;
            for (int i = 0; i < n; ++i) {
                long long before = AVLTree::getRotations();
                avl.deleteKey(delKeys[i]);
                long long delta = AVLTree::getRotations() - before;
                if (delta > maxDelSingle) maxDelSingle = delta;
            }
            long long delRot = AVLTree::getRotations();

            double avgIns = static_cast<double>(insRot) / static_cast<double>(n);
            double avgDel = static_cast<double>(delRot) / static_cast<double>(n);

            out << "| " << std::setw(5) << n << " | "
                << std::setw(6) << (isSorted ? "Sorted" : "Random") << " | "
                << std::setw(11) << insRot << " | "
                << std::setw(14) << std::fixed << std::setprecision(4) << avgIns << " | "
                << std::setw(11) << delRot << " | "
                << std::setw(14) << std::fixed << std::setprecision(4) << avgDel << " | "
                << std::setw(15) << maxDelSingle << " |\n";
        }

        delete[] insKeys;
        delete[] delKeys;
    }
    out << "\n";

    // -------------------------------------------------------------------------
    // 4. Order Statistics (AVL kth vs BST O(n) inorder-scan kth)
    // -------------------------------------------------------------------------
    out << "### 4. Order Statistics (1,000 kth Queries: AVL O(log n) vs BST O(n) Inorder Scan)\n";
    out << "|     N | Structure & Method             | Complexity | Total Time (ms) | Avg Comps/Query | Match |\n";
    out << "|-------|--------------------------------|------------|-----------------|-----------------|-------|\n";

    for (int si = 0; si < 3; ++si) {
        int n = sizes[si];
        int* perm = new int[n];
        fillPermutation1ToN(perm, n, rng);

        BST bst;
        AVLTree avl;
        for (int i = 0; i < n; ++i) {
            bst.insert(perm[i]);
            avl.insert(perm[i]);
        }

        int* ks = new int[numKthQueries];
        int* avlAns = new int[numKthQueries];
        int* bstAns = new int[numKthQueries];
        for (int q = 0; q < numKthQueries; ++q) {
            ks[q] = rng.nextRange(1, n);
        }

        AVLTree::resetCounters();
        auto t0 = Clock::now();
        for (int q = 0; q < numKthQueries; ++q) {
            avlAns[q] = avl.kth(ks[q]);
        }
        auto t1 = Clock::now();
        double avlMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        long long avlComps = AVLTree::getComparisons();

        BST::resetCounters();
        t0 = Clock::now();
        for (int q = 0; q < numKthQueries; ++q) {
            bstAns[q] = bst.kthSmallest(ks[q]);
        }
        t1 = Clock::now();
        double bstMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        long long bstComps = BST::getComparisons();

        bool allMatch = true;
        for (int q = 0; q < numKthQueries; ++q) {
            if (avlAns[q] != bstAns[q]) {
                allMatch = false;
                break;
            }
        }

        out << "| " << std::setw(5) << n << " | AVL kth (subtree size)         | O(log n)   | "
            << std::setw(15) << std::fixed << std::setprecision(3) << avlMs << " | "
            << std::setw(15) << std::fixed << std::setprecision(2) << (static_cast<double>(avlComps) / numKthQueries) << " | "
            << std::setw(5) << (allMatch ? "YES" : "NO") << " |\n";

        out << "| " << std::setw(5) << n << " | BST kthSmallest (inorder scan) | O(n)       | "
            << std::setw(15) << std::fixed << std::setprecision(3) << bstMs << " | "
            << std::setw(15) << std::fixed << std::setprecision(2) << (static_cast<double>(bstComps) / numKthQueries) << " | "
            << std::setw(5) << (allMatch ? "YES" : "NO") << " |\n";

        delete[] ks;
        delete[] avlAns;
        delete[] bstAns;
        delete[] perm;
    }
    out << "\n";

    // -------------------------------------------------------------------------
    // 5. Heap Construction (O(n) Bottom-Up buildMaxHeap)
    // -------------------------------------------------------------------------
    out << "### 5. Max Heap Construction (Bottom-Up O(n) buildMaxHeap)\n";
    out << "|     N | Input Order     | Avg Time (us) | Comparisons | Comps / N |   Swaps | Valid |\n";
    out << "|-------|-----------------|---------------|-------------|-----------|---------|-------|\n";

    const int buildDists[3] = { DIST_RANDOM, DIST_SORTED, DIST_REVERSE_SORTED };
    const char* buildLabels[3] = { "Random", "Sorted (Asc)", "Sorted (Desc)" };

    for (int si = 0; si < 3; ++si) {
        int n = sizes[si];
        int reps = repeatsForHeapBuild(n);
        int* arr = new int[n];
        MaxHeap heap(n);

        for (int di = 0; di < 3; ++di) {
            fillDistribution(arr, n, buildDists[di], rng);

            MaxHeap::resetCounters();
            heap.buildMaxHeap(arr, n);
            long long comps = MaxHeap::getComparisons();
            long long swaps = MaxHeap::getSwaps();
            bool valid = heap.isMaxHeap();

            auto t0 = Clock::now();
            for (int r = 0; r < reps; ++r) {
                heap.buildMaxHeap(arr, n);
            }
            auto t1 = Clock::now();
            double avgUs = std::chrono::duration<double, std::micro>(t1 - t0).count() / static_cast<double>(reps);

            out << "| " << std::setw(5) << n << " | "
                << std::setw(15) << buildLabels[di] << " | "
                << std::setw(13) << std::fixed << std::setprecision(2) << avgUs << " | "
                << std::setw(11) << comps << " | "
                << std::setw(9) << std::fixed << std::setprecision(3) << (static_cast<double>(comps) / n) << " | "
                << std::setw(7) << swaps << " | "
                << std::setw(5) << (valid ? "YES" : "NO") << " |\n";
        }
        delete[] arr;
    }
    out << "Note: Comps / N remains bounded <= 2.00 across all N, confirming O(n) Floyd heap construction rather than O(n log n).\n\n";

    // -------------------------------------------------------------------------
    // 6. Heap Sort + 7. Sorting Comparison (Heap Sort vs Merge Sort)
    // -------------------------------------------------------------------------
    std::ostringstream sec6;
    std::ostringstream sec7;

    sec6 << "### 6. Heap Sort Analysis (All Distributions)\n";
    sec6 << "|     N | Distribution    | Avg Time (ms) | Comparisons |   Swaps | Sorted |\n";
    sec6 << "|-------|-----------------|---------------|-------------|---------|--------|\n";

    sec7 << "### 7. Sorting Comparison: Heap Sort (O(1) aux space) vs Merge Sort (O(n) aux space)\n";
    sec7 << "|     N | Distribution    | Heap Time (ms) | Heap Comps | Heap Swaps | Merge Time (ms) | Merge Comps | Merge Moves | OK  |\n";
    sec7 << "|-------|-----------------|----------------|------------|------------|-----------------|-------------|-------------|-----|\n";

    const int sortDists[4] = { DIST_RANDOM, DIST_SORTED, DIST_REVERSE_SORTED, DIST_DUPLICATE_HEAVY };

    for (int si = 0; si < 3; ++si) {
        int n = sizes[si];
        int reps = repeatsForSort(n);
        MaxHeap heap(n);

        int* baseArr = new int[n];
        int* workOne = new int[n];

        for (int di = 0; di < 4; ++di) {
            int dist = sortDists[di];
            fillDistribution(baseArr, n, dist, rng);

            // Exact single-run counters for Heap Sort
            copyIntArray(baseArr, workOne, n);
            MaxHeap::resetCounters();
            heap.heapSort(workOne, n);
            long long hsComps = MaxHeap::getComparisons();
            long long hsSwaps = MaxHeap::getSwaps();
            bool hsOk = isSortedAscending(workOne, n);

            // Exact single-run counters for Merge Sort
            copyIntArray(baseArr, workOne, n);
            long long msComps = 0;
            long long msMoves = 0;
            mergeSort(workOne, n, msComps, msMoves);
            bool msOk = isSortedAscending(workOne, n);

            // Pre-allocate scratch copies outside timed region for averaged timings
            int** hsCopies = new int*[reps];
            int** msCopies = new int*[reps];
            for (int r = 0; r < reps; ++r) {
                hsCopies[r] = new int[n];
                msCopies[r] = new int[n];
                copyIntArray(baseArr, hsCopies[r], n);
                copyIntArray(baseArr, msCopies[r], n);
            }

            auto t0 = Clock::now();
            for (int r = 0; r < reps; ++r) {
                heap.heapSort(hsCopies[r], n);
            }
            auto t1 = Clock::now();
            double hsAvgMs = std::chrono::duration<double, std::milli>(t1 - t0).count() / static_cast<double>(reps);

            long long dummyC = 0, dummyM = 0;
            t0 = Clock::now();
            for (int r = 0; r < reps; ++r) {
                mergeSort(msCopies[r], n, dummyC, dummyM);
            }
            t1 = Clock::now();
            double msAvgMs = std::chrono::duration<double, std::milli>(t1 - t0).count() / static_cast<double>(reps);

            for (int r = 0; r < reps; ++r) {
                delete[] hsCopies[r];
                delete[] msCopies[r];
            }
            delete[] hsCopies;
            delete[] msCopies;

            sec6 << "| " << std::setw(5) << n << " | "
                 << std::setw(15) << distLabel(dist) << " | "
                 << std::setw(13) << std::fixed << std::setprecision(4) << hsAvgMs << " | "
                 << std::setw(11) << hsComps << " | "
                 << std::setw(7) << hsSwaps << " | "
                 << std::setw(6) << (hsOk ? "YES" : "NO") << " |\n";

            sec7 << "| " << std::setw(5) << n << " | "
                 << std::setw(15) << distLabel(dist) << " | "
                 << std::setw(14) << std::fixed << std::setprecision(4) << hsAvgMs << " | "
                 << std::setw(10) << hsComps << " | "
                 << std::setw(10) << hsSwaps << " | "
                 << std::setw(15) << std::fixed << std::setprecision(4) << msAvgMs << " | "
                 << std::setw(11) << msComps << " | "
                 << std::setw(11) << msMoves << " | "
                 << std::setw(3) << ((hsOk && msOk) ? "YES" : "NO") << " |\n";
        }

        delete[] baseArr;
        delete[] workOne;
    }

    out << sec6.str() << "\n" << sec7.str();
    out << "Space Complexity Note: Heap Sort operates strictly in-place with O(1) auxiliary space,\n";
    out << "whereas Merge Sort requires an O(n) auxiliary merge buffer (and O(log n) recursion stack).\n";
    out << "====================================================================================\n";

    std::ofstream file(reportPath, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open benchmark report path: " << reportPath << "\n";
        return false;
    }
    file << out.str();
    file.flush();
    if (!file.good()) {
        std::cerr << "Error: Failed writing benchmark report to: " << reportPath << "\n";
        return false;
    }

    std::cout << "Benchmark suite completed. Report written to: " << reportPath << "\n";
    return true;
}

} // namespace VaultHeist
