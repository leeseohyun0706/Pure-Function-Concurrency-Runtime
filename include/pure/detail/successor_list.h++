
#ifndef CONCAL_SUCCESSOR_LIST_H
#define CONCAL_SUCCESSOR_LIST_H
#include <atomic>

class Task;
// Can I utilize memoization?
struct EdgeNode {
    Task* child = nullptr; // Vertex
    EdgeNode* next = nullptr; // Link
};

// Single Lock-Free Linked List to manage input dependency
class SuccessorList {
private:
    static inline EdgeNode _closed{}; // Static reference of last edge (to avoid using nullptr).

    std::atomic<EdgeNode*> _head{nullptr};

    static EdgeNode* closed() noexcept { return &_closed; }

public:
    SuccessorList() = default;
    SuccessorList(const SuccessorList&) = delete;
    SuccessorList& operator=(const SuccessorList&) = delete;

    bool try_attach(EdgeNode& node) {
        auto* head = _head.load(std::memory_order_acquire);

        while (head != closed()) {
            node.next = head;

            if (_head.compare_exchange_weak(head, &node, std::memory_order_acq_rel, std::memory_order_acquire)) return true;
        }

        return false;
    }

    template<typename Visitor> requires std::is_invocable_v<Visitor&, Task&>
    bool close_and_visit(Visitor&& visit) {

        auto *node = _head.exchange(closed(), std::memory_order_acq_rel);

        if (node == closed()) return false;

        while (node != nullptr) {
            auto* next = node->next;
            auto* child = node->child;

            visit(*child);

            node = next;
        }
        return true;
    }
};

#endif //CONCAL_SUCCESSOR_LIST_H
