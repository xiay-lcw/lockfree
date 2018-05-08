#include <atomic>
#include <memory>
#include <vector>
#include <stack>

namespace lockfree
{

// Single link node, used for single-linked list, stack
template<typename T>
struct SLNode {
    std::atomic<SLNode<T>*> next;
    T data;

    SLNode() : next(nullptr) {}
    SLNode(T data) : next(nullptr), data(data) {}
};

template<typename DataType>
class Stack
{
private:
    typedef SLNode<DataType> Node;

    struct Head
    {
        Node*       top;
        uint64_t    counter;    // pop counter, used to solve ABA problem
    };

private:
    std::atomic<Head> m_head;

public:
    Stack() : m_head(Head{nullptr, 0})
    {
    }

    void push(DataType data)
    {
        Node* node = new Node(data);
        Head oldHead;
        do
        {
            oldHead = m_head.load();
            node->next = oldHead.top;
        }
        while (!m_head.compare_exchange_strong(oldHead, Head{node, oldHead.counter}));
    }

    bool pop(DataType& data)
    {
        Head oldHead;
        do
        {
            if (!fetchTop(oldHead)) { return false; }
        }
        while (!m_head.compare_exchange_strong(oldHead, Head{oldHead.top->next.load(), oldHead.counter+1}));

        data = oldHead.top->data;
        delete oldHead.top;
        return true;
    }

    bool popAll(std::vector<DataType>& vec)
    {
        std::unique_ptr<Node> node(peelOffAll());

        if (node == nullptr) { return false; }

        do
        {
            vec.push_back(node->data);
            node.reset(node->next.load());
        }
        while (node != nullptr);

        return true;
    }

    bool popAllBackwards(std::stack<DataType>& stk)
    {
        std::unique_ptr<Node> node(peelOffAll());

        if (node == nullptr) { return false; }

        do
        {
            stk.push(node->data);
            node.reset(node->next.load());
        }
        while (node != nullptr);

        return true;
    }

    bool getTop(DataType& data) const
    {
        Head head;
        bool ret = fetchTop(head);
        if (ret) { data = head.top->data; }
        return ret;
    }

    bool empty() const
    {
        return m_head.load().top == nullptr;
    }

    bool isLockFree() const
    {
        return m_head.is_lock_free();
    }

private:
    inline bool fetchTop(Head& head) const
    {
        return (head = m_head.load()).top != nullptr;
    }

    inline Node* peelOffAll()
    {
        Head oldHead;
        do
        {
            if(!fetchTop(oldHead)) { return nullptr; }
        }
        while (!m_head.compare_exchange_strong(oldHead, Head{nullptr, oldHead.counter}));

        return oldHead.top;
    }
};

};
