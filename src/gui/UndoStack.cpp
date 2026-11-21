#include "gui/UndoStack.h"

namespace VaultHeist {

UndoStack::UndoStack() : count(0) {}

void UndoStack::clear() {
    count = 0;
}

void UndoStack::push(const GameSnapshot& snap) {
    if (count >= MAX_UNDO_SNAPSHOTS) {
        for (int i = 1; i < MAX_UNDO_SNAPSHOTS; ++i) {
            frames[i - 1] = frames[i];
        }
        count = MAX_UNDO_SNAPSHOTS - 1;
    }
    frames[count++] = snap;
}

bool UndoStack::pop(GameSnapshot& out) {
    if (count <= 0) return false;
    out = frames[--count];
    return true;
}

void UndoStack::serializeTreeWithNulls(const TreeNode* node, int* out, int& len) {
    if (len >= MAX_SERIALIZED_LEN) return;
    if (!node) {
        out[len++] = SERIALIZE_NULL_SENTINEL;
        return;
    }
    out[len++] = node->value;
    serializeTreeWithNulls(node->left, out, len);
    serializeTreeWithNulls(node->right, out, len);
}

void UndoStack::serializePreorder(const TreeNode* node, int* out, int& len) {
    if (!node || len >= MAX_SERIALIZED_LEN) return;
    out[len++] = node->value;
    serializePreorder(node->left, out, len);
    serializePreorder(node->right, out, len);
}

void UndoStack::serializeAVLPreorder(const AVLNode* node, int* out, int& len) {
    if (!node || len >= MAX_SERIALIZED_LEN) return;
    out[len++] = node->key;
    serializeAVLPreorder(node->left, out, len);
    serializeAVLPreorder(node->right, out, len);
}

void UndoStack::rebuildTreeFromNullPreorder(BinaryTree& tree, TreeNode* parent, bool asLeft,
                                            const int* arr, int len, int& idx) {
    if (idx >= len) return;
    int val = arr[idx++];
    if (val == SERIALIZE_NULL_SENTINEL) return;

    TreeNode* created = tree.addNode(val, parent, asLeft);
    rebuildTreeFromNullPreorder(tree, created, true, arr, len, idx);
    rebuildTreeFromNullPreorder(tree, created, false, arr, len, idx);
}

void UndoStack::captureStructures(const BinaryTree& bt, const BST& bst,
                                  const AVLTree& avl, const MaxHeap& heap,
                                  const int* rawHeapArr, int rawHeapSize,
                                  const int* sortedArr, int sortedCount,
                                  bool useRealHeap, GameSnapshot& snap) {
    snap.btLen = 0;
    serializeTreeWithNulls(bt.getRoot(), snap.btPre, snap.btLen);

    snap.bstLen = 0;
    serializePreorder(bst.getRoot(), snap.bstPre, snap.bstLen);

    snap.avlLen = 0;
    serializeAVLPreorder(avl.getRoot(), snap.avlPre, snap.avlLen);

    snap.useRealHeap = useRealHeap;
    snap.heapLen = 0;
    if (useRealHeap) {
        int n = heap.getSize();
        if (n > MAX_SERIALIZED_LEN) n = MAX_SERIALIZED_LEN;
        for (int i = 0; i < n; ++i) {
            snap.heapData[snap.heapLen++] = heap.getElementAt(i);
        }
    } else if (rawHeapArr) {
        int n = rawHeapSize;
        if (n > MAX_SERIALIZED_LEN) n = MAX_SERIALIZED_LEN;
        for (int i = 0; i < n; ++i) {
            snap.heapData[snap.heapLen++] = rawHeapArr[i];
        }
    }

    snap.sortedLen = 0;
    if (sortedArr) {
        int s = sortedCount;
        if (s > MAX_SERIALIZED_LEN) s = MAX_SERIALIZED_LEN;
        for (int i = 0; i < s; ++i) {
            snap.sortedData[snap.sortedLen++] = sortedArr[i];
        }
    }
}

void UndoStack::restoreStructures(const GameSnapshot& snap,
                                  BinaryTree& bt, BST& bst,
                                  AVLTree& avl, MaxHeap& heap,
                                  int* rawHeapArr, int& rawHeapSize,
                                  int* sortedArr, int& sortedCount) {
    bt.clear();
    int idx = 0;
    rebuildTreeFromNullPreorder(bt, nullptr, true, snap.btPre, snap.btLen, idx);

    bst.clear();
    for (int i = 0; i < snap.bstLen; ++i) {
        bst.insert(snap.bstPre[i]);
    }

    avl.clear();
    for (int i = 0; i < snap.avlLen; ++i) {
        avl.insert(snap.avlPre[i]);
    }

    if (snap.useRealHeap) {
        if (snap.heapLen > 0) {
            heap.buildMaxHeap(snap.heapData, snap.heapLen);
        } else {
            while (!heap.isEmpty()) heap.extractMax();
        }
    } else if (rawHeapArr) {
        rawHeapSize = snap.heapLen;
        for (int i = 0; i < snap.heapLen; ++i) {
            rawHeapArr[i] = snap.heapData[i];
        }
    }

    if (sortedArr) {
        sortedCount = snap.sortedLen;
        for (int i = 0; i < snap.sortedLen; ++i) {
            sortedArr[i] = snap.sortedData[i];
        }
    }
}

} // namespace VaultHeist
