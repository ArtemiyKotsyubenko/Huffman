#ifndef HUFFMAN_V2_0_HUFFMAN_TREE_H
#define HUFFMAN_V2_0_HUFFMAN_TREE_H

#include <memory>
#include <list>
#include <vector>
#include <stack>

class Huffman_Tree_adapter;

class Huffman_Tree {
private:
    unsigned int current_empty_value = 255;

    enum class Side {
        top = -1,
        left = 0,
        right = 1

    };

    struct Symbol {
        const unsigned int &symbol;
        unsigned int &count;

        Symbol(const unsigned int &symbol_, unsigned int &count_) : symbol(symbol_), count(count_) {

        }
    };

    struct Node {
        std::shared_ptr<Node> left_child = nullptr;
        std::shared_ptr<Node> right_child = nullptr;
        std::weak_ptr<Node> parent;
        const unsigned int symbol;
        unsigned int count = 0;
        Side side;

        Node(const unsigned int symbol_, Side side_) : symbol(symbol_), side(side_) {}
    };

    struct Data {
        std::weak_ptr<Node> ptr;
        std::list<Symbol>::iterator iter;

        bool exists() const {
            return !ptr.expired();
        }

        Data() = default;

    };

    std::list<Symbol> lst;
    std::vector<Data> symbols_in_stock;
    std::shared_ptr<Node> root;
    std::weak_ptr<Node> last_leave;

    inline void swap_nodes(const unsigned int first_symbol, const unsigned int second_symbol) {
        auto first_node = symbols_in_stock[first_symbol].ptr.lock();
        auto second_node = symbols_in_stock[second_symbol].ptr.lock();

        //iterators will point correctly again. Swapping iterators in enough because
        //list's values are references, so they always contains values of Node from which they were constructed
        std::swap(symbols_in_stock[first_symbol].iter, symbols_in_stock[second_symbol].iter);

        //changing parent's counter
        first_node->parent.lock()->count += (second_node->count - first_node->count);// ошибка и еще раз
        second_node->parent.lock()->count += (first_node->count - second_node->count);

        //swap Nodes in tree
        std::swap((first_node->side == Side::left) ?
                  first_node->parent.lock()->left_child :
                  first_node->parent.lock()->right_child,
                  (second_node->side == Side::left) ?
                  second_node->parent.lock()->left_child :
                  second_node->parent.lock()->right_child
        );

        //set sides correctly
        std::swap(first_node->side, second_node->side);

    }


    //increases inserted element's counter and restores Huffman's tree traits
    inline void increase_and_rebalance(const unsigned int symbol) {
        /*
         * __while root is not reached__:
         * incrementation counter of inserted symbol
         * check if list after incrementation satisfies Huffman's tree traits
         * if satisfies - check symbol's parent*
         * if conditions were violated:
         * 1) go left in the list while current element greater then previous
         * 2) swap current and element from '1)' in list and in the tree
         * __again__
         * */
        auto current = symbols_in_stock[symbol].ptr.lock();
        while (current != root) {

            auto iter = symbols_in_stock[symbol].iter;

            if ((--iter)->count + 1 < (++iter)->count) {
                auto next_iter = iter;

                while (next_iter--->count > next_iter->count + 1);
                //лишний прогон чинится инкрементом
                ++current->count;
                swap_nodes(iter->symbol, (++next_iter)->symbol);
            } else {
                ++current->count;
                current = current->parent.lock();
            }
        }
        ++root->count;


    }

    inline void insert_new(const unsigned int symbol) {
        std::shared_ptr<Node> pv = std::make_shared<Node>(symbol, Side::right);
        std::shared_ptr<Node> leave = std::make_shared<Node>(++current_empty_value, Side::left);
        pv->parent = last_leave.lock();
        leave->parent = last_leave.lock();
        last_leave.lock()->right_child = pv;
        last_leave.lock()->left_child = leave;
        last_leave = last_leave.lock()->left_child;

        symbols_in_stock[symbol].ptr = pv;
        symbols_in_stock[current_empty_value - 1].ptr = leave;

        symbols_in_stock[symbol].iter = lst.emplace(lst.end(), pv->symbol, pv->count);

        symbols_in_stock[current_empty_value - 1].iter = lst.emplace(lst.end(), leave->symbol, leave->count);


        increase_and_rebalance(symbol);


    }

    inline void insert_existing(const unsigned int symbol) {
        increase_and_rebalance(symbol);

    }

public:
    friend Huffman_Tree_adapter;

    Huffman_Tree() : symbols_in_stock(513) {
        root = std::make_shared<Node>(++current_empty_value, Side::top);
        lst.emplace(lst.begin(), root->symbol, root->count);
        last_leave = root;

    }

    void insert(const unsigned int symbol) {
        if (symbols_in_stock[symbol].exists()) {
            insert_existing(symbol);
        } else {
            insert_new(symbol);
        }
    }

    std::vector<unsigned char> get_symbol_code(const unsigned char symbol) const {
        auto pv = symbols_in_stock[symbol].ptr.lock();

        std::stack<Side> st;
        std::vector<unsigned char> symbol_code;

        while (pv != root) {
            st.push((pv->side == Huffman_Tree::Side::left) ? Side::left : Side::right);
            pv = pv->parent.lock();
        }

        while (!st.empty()) {
            symbol_code.push_back(static_cast<const unsigned char>(st.top()));
            st.pop();
        }

        return symbol_code;
    }

    std::vector<unsigned char> get_delim_code() const {
        auto pv = last_leave.lock();

        std::stack<Side> st;
        std::vector<unsigned char> symbol_code;

        while (pv != root) {
            st.push((pv->side == Side::left) ? Side::left : Side::right);
            pv = pv->parent.lock();
        }

        while (!st.empty()) {
            symbol_code.push_back(static_cast<const unsigned char>(st.top()));
            st.pop();
        }

        return symbol_code;
    }

    bool exists(const unsigned char symbol) const {
        return symbols_in_stock[symbol].exists();
    }

    ~Huffman_Tree() = default;

};

class Huffman_Tree_adapter {
private:
    Huffman_Tree &tree;
    std::weak_ptr<Huffman_Tree::Node> ptr;
public:
    Huffman_Tree_adapter(Huffman_Tree &tree_) : tree(tree_), ptr(tree.root) {}

    ~Huffman_Tree_adapter() = default;

    // moves to left child when bit = 0 and to right child when bit = 1
    bool move(const bool bit) {
        if (bit) {
            ptr = ptr.lock()->right_child;
        } else {
            ptr = ptr.lock()->left_child;
        }

        return (ptr.lock()->symbol < 256 || ptr.lock() == tree.last_leave.lock());
    }

    // should be called when 'move()' reached leave, returns 'true' if leave contains symbol and 'false' if delimiter
    bool is_symbol_code() const {
        return ptr.lock()->symbol < 256;
    }

    // should be called when 'is_symbol_code()' returned 'true' - returns symbol of ptr's Node
    unsigned char get_symbol() const {
        unsigned char symbol = ptr.lock()->symbol;
        return symbol;
    }

    //ptr will point to root
    void rewind() {
        ptr = tree.root;
    }

};

#endif //HUFFMAN_V2_0_HUFFMAN_TREE_H
