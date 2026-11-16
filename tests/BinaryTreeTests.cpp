#include <gtest/gtest.h>
#include "structures/BinaryTree.h"

using namespace VaultHeist;

// 1. Empty Tree Behavior
TEST(BinaryTreeTest, EmptyTreeOperations) {
    BinaryTree tree;
    EXPECT_TRUE(tree.isEmpty());
    EXPECT_EQ(tree.getSize(), 0);
    EXPECT_EQ(tree.height(), 0);
    EXPECT_EQ(tree.diameter(), 0);
    EXPECT_TRUE(tree.isSymmetric());

    int count = 0;
    int* preRec = tree.preorderRecursive(count);
    EXPECT_EQ(preRec, nullptr);
    EXPECT_EQ(count, 0);

    int* inRec = tree.inorderRecursive(count);
    EXPECT_EQ(inRec, nullptr);
    EXPECT_EQ(count, 0);

    int* postRec = tree.postorderRecursive(count);
    EXPECT_EQ(postRec, nullptr);
    EXPECT_EQ(count, 0);

    int* preIter = tree.preorderIterative(count);
    EXPECT_EQ(preIter, nullptr);
    EXPECT_EQ(count, 0);

    int* inIter = tree.inorderIterative(count);
    EXPECT_EQ(inIter, nullptr);
    EXPECT_EQ(count, 0);

    int* postIter = tree.postorderIterative(count);
    EXPECT_EQ(postIter, nullptr);
    EXPECT_EQ(count, 0);

    int* lvl = tree.levelOrder(count);
    EXPECT_EQ(lvl, nullptr);
    EXPECT_EQ(count, 0);

    EXPECT_EQ(tree.lca(1, 2), -1);

    int pathLen = 0;
    int* path = tree.pathTo(1, pathLen);
    EXPECT_EQ(path, nullptr);
    EXPECT_EQ(pathLen, 0);
}

// 2. Single Node Tree
TEST(BinaryTreeTest, SingleNodeTree) {
    BinaryTree tree;
    TreeNode* root = tree.addNode(42, nullptr, true);
    ASSERT_NE(root, nullptr);
    EXPECT_FALSE(tree.isEmpty());
    EXPECT_EQ(tree.getSize(), 1);
    EXPECT_EQ(tree.height(), 1);
    EXPECT_EQ(tree.diameter(), 1);
    EXPECT_TRUE(tree.isSymmetric());

    int count = 0;
    int* preRec = tree.preorderRecursive(count);
    ASSERT_NE(preRec, nullptr);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(preRec[0], 42);
    delete[] preRec;

    int* preIter = tree.preorderIterative(count);
    ASSERT_NE(preIter, nullptr);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(preIter[0], 42);
    delete[] preIter;

    int* inRec = tree.inorderRecursive(count);
    ASSERT_NE(inRec, nullptr);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(inRec[0], 42);
    delete[] inRec;

    int* inIter = tree.inorderIterative(count);
    ASSERT_NE(inIter, nullptr);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(inIter[0], 42);
    delete[] inIter;

    int* postRec = tree.postorderRecursive(count);
    ASSERT_NE(postRec, nullptr);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(postRec[0], 42);
    delete[] postRec;

    int* postIter = tree.postorderIterative(count);
    ASSERT_NE(postIter, nullptr);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(postIter[0], 42);
    delete[] postIter;

    int* lvl = tree.levelOrder(count);
    ASSERT_NE(lvl, nullptr);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(lvl[0], 42);
    delete[] lvl;

    EXPECT_EQ(tree.lca(42, 42), 42);
    EXPECT_EQ(tree.lca(42, 99), -1);

    int pathLen = 0;
    int* path = tree.pathTo(42, pathLen);
    ASSERT_NE(path, nullptr);
    EXPECT_EQ(pathLen, 1);
    EXPECT_EQ(path[0], 42);
    delete[] path;

    path = tree.pathTo(99, pathLen);
    EXPECT_EQ(path, nullptr);
    EXPECT_EQ(pathLen, 0);
}

// 3. Skewed Tree (All-Left and All-Right)
TEST(BinaryTreeTest, SkewedTreeTraversalsMatch) {
    // All-left skewed tree: 4 -> 3 -> 2 -> 1
    BinaryTree leftSkewed;
    TreeNode* n4 = leftSkewed.addNode(4, nullptr, true);
    TreeNode* n3 = leftSkewed.addNode(3, n4, true);
    TreeNode* n2 = leftSkewed.addNode(2, n3, true);
    leftSkewed.addNode(1, n2, true);

    EXPECT_EQ(leftSkewed.getSize(), 4);
    EXPECT_EQ(leftSkewed.height(), 4);
    EXPECT_EQ(leftSkewed.diameter(), 4);

    int c1 = 0, c2 = 0;
    int* preR = leftSkewed.preorderRecursive(c1);
    int* preI = leftSkewed.preorderIterative(c2);
    ASSERT_EQ(c1, c2);
    EXPECT_EQ(c1, 4);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(preR[i], preI[i]);
    delete[] preR;
    delete[] preI;

    int* inR = leftSkewed.inorderRecursive(c1);
    int* inI = leftSkewed.inorderIterative(c2);
    ASSERT_EQ(c1, c2);
    EXPECT_EQ(c1, 4);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(inR[i], inI[i]);
    delete[] inR;
    delete[] inI;

    int* postR = leftSkewed.postorderRecursive(c1);
    int* postI = leftSkewed.postorderIterative(c2);
    ASSERT_EQ(c1, c2);
    EXPECT_EQ(c1, 4);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(postR[i], postI[i]);
    delete[] postR;
    delete[] postI;

    // All-right skewed tree: 10 -> 20 -> 30
    BinaryTree rightSkewed;
    TreeNode* r10 = rightSkewed.addNode(10, nullptr, false);
    TreeNode* r20 = rightSkewed.addNode(20, r10, false);
    rightSkewed.addNode(30, r20, false);

    EXPECT_EQ(rightSkewed.getSize(), 3);
    EXPECT_EQ(rightSkewed.height(), 3);
    EXPECT_EQ(rightSkewed.diameter(), 3);

    int* rPreR = rightSkewed.preorderRecursive(c1);
    int* rPreI = rightSkewed.preorderIterative(c2);
    ASSERT_EQ(c1, c2);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(rPreR[i], rPreI[i]);
    delete[] rPreR;
    delete[] rPreI;

    int* rInR = rightSkewed.inorderRecursive(c1);
    int* rInI = rightSkewed.inorderIterative(c2);
    ASSERT_EQ(c1, c2);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(rInR[i], rInI[i]);
    delete[] rInR;
    delete[] rInI;

    int* rPostR = rightSkewed.postorderRecursive(c1);
    int* rPostI = rightSkewed.postorderIterative(c2);
    ASSERT_EQ(c1, c2);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(rPostR[i], rPostI[i]);
    delete[] rPostR;
    delete[] rPostI;
}

// 4. Balanced Tree Traversals & Level Order
TEST(BinaryTreeTest, BalancedTreeTraversalsAndLevelOrder) {
    //        1
    //      /   \
    //     2     3
    //    / \   / \
    //   4   5 6   7
    BinaryTree tree;
    TreeNode* n1 = tree.addNode(1, nullptr, true);
    TreeNode* n2 = tree.addNode(2, n1, true);
    TreeNode* n3 = tree.addNode(3, n1, false);
    tree.addNode(4, n2, true);
    tree.addNode(5, n2, false);
    tree.addNode(6, n3, true);
    tree.addNode(7, n3, false);

    EXPECT_EQ(tree.getSize(), 7);
    EXPECT_EQ(tree.height(), 3);
    EXPECT_EQ(tree.diameter(), 5);

    int c1 = 0, c2 = 0;
    int* preR = tree.preorderRecursive(c1);
    int* preI = tree.preorderIterative(c2);
    ASSERT_EQ(c1, 7);
    ASSERT_EQ(c1, c2);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(preR[i], preI[i]);

    int* inR = tree.inorderRecursive(c1);
    int* inI = tree.inorderIterative(c2);
    ASSERT_EQ(c1, 7);
    ASSERT_EQ(c1, c2);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(inR[i], inI[i]);

    int* postR = tree.postorderRecursive(c1);
    int* postI = tree.postorderIterative(c2);
    ASSERT_EQ(c1, 7);
    ASSERT_EQ(c1, c2);
    for (int i = 0; i < c1; ++i) EXPECT_EQ(postR[i], postI[i]);

    int* lvl = tree.levelOrder(c1);
    ASSERT_EQ(c1, 7);
    int expectedLvl[] = {1, 2, 3, 4, 5, 6, 7};
    for (int i = 0; i < 7; ++i) EXPECT_EQ(lvl[i], expectedLvl[i]);

    delete[] preR;
    delete[] preI;
    delete[] inR;
    delete[] inI;
    delete[] postR;
    delete[] postI;
    delete[] lvl;
}

// 5. Reconstruction from Preorder + Inorder
TEST(BinaryTreeTest, ReconstructionRoundTripPreIn) {
    BinaryTree original;
    TreeNode* n1 = original.addNode(1, nullptr, true);
    TreeNode* n2 = original.addNode(2, n1, true);
    TreeNode* n3 = original.addNode(3, n1, false);
    original.addNode(4, n2, true);
    original.addNode(5, n2, false);
    original.addNode(6, n3, true);
    original.addNode(7, n3, false);

    int preCount = 0, inCount = 0;
    int* pre = original.preorderRecursive(preCount);
    int* in  = original.inorderRecursive(inCount);
    ASSERT_EQ(preCount, 7);
    ASSERT_EQ(inCount, 7);

    BinaryTree rebuilt;
    rebuilt.buildFromPreorderInorder(pre, in, preCount);

    EXPECT_EQ(rebuilt.getSize(), original.getSize());
    EXPECT_EQ(rebuilt.height(), original.height());
    EXPECT_TRUE(rebuilt.isSameTree(original));

    delete[] pre;
    delete[] in;
}

// 6. Reconstruction from Postorder + Inorder
TEST(BinaryTreeTest, ReconstructionRoundTripPostIn) {
    BinaryTree original;
    TreeNode* n1 = original.addNode(1, nullptr, true);
    TreeNode* n2 = original.addNode(2, n1, true);
    TreeNode* n3 = original.addNode(3, n1, false);
    original.addNode(4, n2, true);
    original.addNode(5, n2, false);
    original.addNode(6, n3, true);
    original.addNode(7, n3, false);

    int postCount = 0, inCount = 0;
    int* post = original.postorderRecursive(postCount);
    int* in   = original.inorderRecursive(inCount);
    ASSERT_EQ(postCount, 7);
    ASSERT_EQ(inCount, 7);

    BinaryTree rebuilt;
    rebuilt.buildFromPostorderInorder(post, in, postCount);

    EXPECT_EQ(rebuilt.getSize(), original.getSize());
    EXPECT_EQ(rebuilt.height(), original.height());
    EXPECT_TRUE(rebuilt.isSameTree(original));

    delete[] post;
    delete[] in;
}

// 7. LCA and PathTo
TEST(BinaryTreeTest, LCAAndPathTo) {
    //        1
    //      /   \
    //     2     3
    //    / \     \
    //   4   5     6
    BinaryTree tree;
    TreeNode* n1 = tree.addNode(1, nullptr, true);
    TreeNode* n2 = tree.addNode(2, n1, true);
    TreeNode* n3 = tree.addNode(3, n1, false);
    tree.addNode(4, n2, true);
    tree.addNode(5, n2, false);
    tree.addNode(6, n3, false);

    // LCA tests
    EXPECT_EQ(tree.lca(4, 5), 2);
    EXPECT_EQ(tree.lca(4, 6), 1);
    EXPECT_EQ(tree.lca(2, 4), 2);
    EXPECT_EQ(tree.lca(1, 6), 1);
    EXPECT_EQ(tree.lca(4, 99), -1);
    EXPECT_EQ(tree.lca(99, 100), -1);

    // PathTo tests
    int len = 0;
    int* path5 = tree.pathTo(5, len);
    ASSERT_NE(path5, nullptr);
    EXPECT_EQ(len, 3);
    EXPECT_EQ(path5[0], 1);
    EXPECT_EQ(path5[1], 2);
    EXPECT_EQ(path5[2], 5);
    delete[] path5;

    int* path6 = tree.pathTo(6, len);
    ASSERT_NE(path6, nullptr);
    EXPECT_EQ(len, 3);
    EXPECT_EQ(path6[0], 1);
    EXPECT_EQ(path6[1], 3);
    EXPECT_EQ(path6[2], 6);
    delete[] path6;

    int* pathMissing = tree.pathTo(99, len);
    EXPECT_EQ(pathMissing, nullptr);
    EXPECT_EQ(len, 0);
}

// 8. Mirror and Symmetry
TEST(BinaryTreeTest, MirrorAndSymmetry) {
    // Build a symmetric tree:
    //        1
    //      /   \
    //     2     2
    //    / \   / \
    //   3   4 4   3
    BinaryTree symTree;
    TreeNode* s1 = symTree.addNode(1, nullptr, true);
    TreeNode* s2L = symTree.addNode(2, s1, true);
    TreeNode* s2R = symTree.addNode(2, s1, false);
    symTree.addNode(3, s2L, true);
    symTree.addNode(4, s2L, false);
    symTree.addNode(4, s2R, true);
    symTree.addNode(3, s2R, false);

    EXPECT_TRUE(symTree.isSymmetric());

    // Asymmetric tree
    BinaryTree asymTree;
    TreeNode* a1 = asymTree.addNode(1, nullptr, true);
    TreeNode* a2 = asymTree.addNode(2, a1, true);
    asymTree.addNode(3, a2, true);
    EXPECT_FALSE(asymTree.isSymmetric());

    // Mirroring a tree twice restores original structure
    BinaryTree t;
    TreeNode* r = t.addNode(10, nullptr, true);
    TreeNode* leftChild = t.addNode(5, r, true);
    t.addNode(20, r, false);
    t.addNode(2, leftChild, true);

    BinaryTree originalCopy;
    TreeNode* cr = originalCopy.addNode(10, nullptr, true);
    TreeNode* cLeft = originalCopy.addNode(5, cr, true);
    originalCopy.addNode(20, cr, false);
    originalCopy.addNode(2, cLeft, true);

    EXPECT_TRUE(t.isSameTree(originalCopy));
    t.mirror();
    EXPECT_FALSE(t.isSameTree(originalCopy));
    t.mirror();
    EXPECT_TRUE(t.isSameTree(originalCopy));
}
