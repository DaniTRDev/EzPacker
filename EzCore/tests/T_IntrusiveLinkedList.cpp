#include "HelperClasses/IntrusiveLinkedList.h"
#include <gtest/gtest.h>
#include <vector>

struct TestNode
{
    int m_value{ 0 };
    TestNode *m_prev{ nullptr };
    TestNode *m_next{ nullptr };

    explicit TestNode(int v = 0) : m_value(v) {}

    TestNode *getPrev() const { return m_prev; }
    void setPrev(TestNode *p) { m_prev = p; }
    TestNode *getNext() const { return m_next; }
    void setNext(TestNode *n) { m_next = n; }
};

static void verifyListIntegrity(const IntrusiveLinkedList<TestNode> &list, const std::vector<int> &expected)
{
    EXPECT_EQ(list.size(), expected.size());
    EXPECT_EQ(list.empty(), expected.empty());

    if (expected.empty())
    {
        EXPECT_EQ(list.front(), nullptr);
        EXPECT_EQ(list.back(), nullptr);
        EXPECT_EQ(list.begin(), list.end());
        return;
    }

    EXPECT_NE(list.front(), nullptr);
    EXPECT_NE(list.back(), nullptr);
    EXPECT_EQ(list.front()->m_value, expected.front());
    EXPECT_EQ(list.back()->m_value, expected.back());

    // Forward traversal
    size_t idx = 0;
    const TestNode *prevNode = nullptr;
    for (auto it = list.begin(); it != list.end(); ++it, ++idx)
    {
        EXPECT_LT(idx, expected.size());
        EXPECT_EQ((*it)->m_value, expected[idx]);
        EXPECT_EQ((*it)->getPrev(), prevNode);
        if (prevNode)
        {
            EXPECT_EQ(prevNode->getNext(), *it);
        }
        prevNode = *it;
    }
    EXPECT_EQ(idx, expected.size());
    EXPECT_EQ(prevNode, list.back());
    EXPECT_EQ(list.back()->getNext(), nullptr);

    // Reverse traversal
    size_t rIdx = expected.size();
    for (auto rIt = list.rbegin(); rIt != list.rend(); ++rIt)
    {
        --rIdx;
        EXPECT_EQ((*rIt)->m_value, expected[rIdx]);
    }
    EXPECT_EQ(rIdx, 0u);
}

TEST(T_IntrusiveLinkedList, TestEmptyListInvariants)
{
    IntrusiveLinkedList<TestNode> list;
    verifyListIntegrity(list, {});

    EXPECT_EQ(list.pop_front(), nullptr);
    EXPECT_EQ(list.pop_back(), nullptr);
    EXPECT_EQ(list.erase(list.end()), list.end());
}

TEST(T_IntrusiveLinkedList, TestSingleElementBoundary)
{
    IntrusiveLinkedList<TestNode> list;
    TestNode n1(42);

    list.push_back(&n1);
    verifyListIntegrity(list, { 42 });

    TestNode *popped = list.pop_front();
    EXPECT_EQ(popped, &n1);
    verifyListIntegrity(list, {});

    list.push_front(&n1);
    verifyListIntegrity(list, { 42 });

    popped = list.pop_back();
    EXPECT_EQ(popped, &n1);
    verifyListIntegrity(list, {});
}

TEST(T_IntrusiveLinkedList, TestPushFrontAndBack)
{
    IntrusiveLinkedList<TestNode> list;
    TestNode n1(1), n2(2), n3(3), n0(0);

    list.push_back(&n1);
    list.push_back(&n2);
    list.push_back(&n3);
    list.push_front(&n0);

    verifyListIntegrity(list, { 0, 1, 2, 3 });
}

TEST(T_IntrusiveLinkedList, TestInsertOperations)
{
    IntrusiveLinkedList<TestNode> list;
    TestNode n1(1), n2(2), n3(3), n10(10), n20(20);

    list.push_back(&n1);
    list.push_back(&n3);

    // Insert n2 before n3 (middle)
    auto it = list.begin();
    ++it; // Points to n3
    list.insert(it, &n2);
    verifyListIntegrity(list, { 1, 2, 3 });

    // Insert n0 before n1 (begin)
    list.insert(list.begin(), &n10);
    verifyListIntegrity(list, { 10, 1, 2, 3 });

    // Insert n20 before end (back)
    list.insert(list.end(), &n20);
    verifyListIntegrity(list, { 10, 1, 2, 3, 20 });
}

TEST(T_IntrusiveLinkedList, TestEraseAndRemove)
{
    IntrusiveLinkedList<TestNode> list;
    TestNode n1(1), n2(2), n3(3), n4(4);

    list.push_back(&n1);
    list.push_back(&n2);
    list.push_back(&n3);
    list.push_back(&n4);

    // Erase middle (n2)
    auto it = list.begin();
    ++it; // n2
    auto nextIt = list.erase(it);
    EXPECT_EQ((*nextIt)->m_value, 3);
    verifyListIntegrity(list, { 1, 3, 4 });

    // Erase head (n1)
    nextIt = list.erase(list.begin());
    EXPECT_EQ((*nextIt)->m_value, 3);
    verifyListIntegrity(list, { 3, 4 });

    // Remove by pointer (n4 - tail)
    list.remove(&n4);
    verifyListIntegrity(list, { 3 });

    // Remove only remaining element
    list.remove(&n3);
    verifyListIntegrity(list, {});
}

TEST(T_IntrusiveLinkedList, TestClear)
{
    IntrusiveLinkedList<TestNode> list;
    TestNode n1(1), n2(2), n3(3);
    list.push_back(&n1);
    list.push_back(&n2);
    list.push_back(&n3);

    list.clear();
    verifyListIntegrity(list, {});
    EXPECT_EQ(n1.getPrev(), nullptr);
    EXPECT_EQ(n1.getNext(), nullptr);
    EXPECT_EQ(n2.getPrev(), nullptr);
    EXPECT_EQ(n2.getNext(), nullptr);
    EXPECT_EQ(n3.getPrev(), nullptr);
    EXPECT_EQ(n3.getNext(), nullptr);
}

TEST(T_IntrusiveLinkedList, TestSpliceEntireList)
{
    // Splice into empty list
    {
        IntrusiveLinkedList<TestNode> l1, l2;
        TestNode a(1), b(2), c(3);
        l2.push_back(&a);
        l2.push_back(&b);
        l2.push_back(&c);

        l1.splice(l1.end(), l2);
        verifyListIntegrity(l1, { 1, 2, 3 });
        verifyListIntegrity(l2, {});
    }

    // Splice at end of non-empty list
    {
        IntrusiveLinkedList<TestNode> l1, l2;
        TestNode a(1), b(2), c(3), d(4);
        l1.push_back(&a);
        l1.push_back(&b);
        l2.push_back(&c);
        l2.push_back(&d);

        l1.splice(l1.end(), l2);
        verifyListIntegrity(l1, { 1, 2, 3, 4 });
        verifyListIntegrity(l2, {});
    }

    // Splice at begin of non-empty list
    {
        IntrusiveLinkedList<TestNode> l1, l2;
        TestNode a(1), b(2), c(3), d(4);
        l1.push_back(&c);
        l1.push_back(&d);
        l2.push_back(&a);
        l2.push_back(&b);

        l1.splice(l1.begin(), l2);
        verifyListIntegrity(l1, { 1, 2, 3, 4 });
        verifyListIntegrity(l2, {});
    }

    // Splice in middle of non-empty list
    {
        IntrusiveLinkedList<TestNode> l1, l2;
        TestNode a(1), b(4), m1(2), m2(3);
        l1.push_back(&a);
        l1.push_back(&b);
        l2.push_back(&m1);
        l2.push_back(&m2);

        auto it = l1.begin();
        ++it; // points to b(4)
        l1.splice(it, l2);
        verifyListIntegrity(l1, { 1, 2, 3, 4 });
        verifyListIntegrity(l2, {});
    }
}

TEST(T_IntrusiveLinkedList, TestSpliceSingleElement)
{
    IntrusiveLinkedList<TestNode> l1, l2;
    TestNode a(1), b(3), x(2);
    l1.push_back(&a);
    l1.push_back(&b);
    l2.push_back(&x);

    auto it = l1.begin();
    ++it; // points to b
    l1.splice(it, l2, l2.begin());

    verifyListIntegrity(l1, { 1, 2, 3 });
    verifyListIntegrity(l2, {});
}

TEST(T_IntrusiveLinkedList, TestSpliceRange)
{
    IntrusiveLinkedList<TestNode> l1, l2;
    TestNode a(1), b(5), x(2), y(3), z(4);
    l1.push_back(&a);
    l1.push_back(&b);
    l2.push_back(&x);
    l2.push_back(&y);
    l2.push_back(&z);

    auto first = l2.begin(); // x
    auto last = l2.end();   // end
    auto pos = l1.begin();
    ++pos; // points to b(5)

    l1.splice(pos, l2, first, last, 3);
    verifyListIntegrity(l1, { 1, 2, 3, 4, 5 });
    verifyListIntegrity(l2, {});
}
