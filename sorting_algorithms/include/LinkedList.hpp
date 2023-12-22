#ifndef __TD_LINKED_LIST_HPP__
#define __TD_LINKED_LIST_HPP__

#include <stddef.h> // std::size_t
#include <exception> // runtime_error
#include <ostream>

namespace TD 
{
    template <typename T>
    class LinkedList
    {
        private:
            struct Node 
            {
                private:
                    T m_data;
                    Node *m_next;

                public:
                    inline Node(T data_, Node *next_);

                    inline T GetData() const;
                    inline bool HasNext() const;
                    inline Node *Next();

                    friend class LinkedList;
            };

        public:
            LinkedList();
            ~LinkedList();
            Node *Insert(Node *iter, T data);
            T Remove(Node *node);
            std::size_t Size();
            bool IsEmpty();
            void Clear();
            void Find(T data);
            Node *Begin() const;
            Node *End() const;

            inline bool IsIterEqual(Node *iter1_, Node *iter2_);

            friend class Iterator;

        private:
            Node *m_head;
            Node *m_tail;

    };  // The end of LinkedList class

    template <typename T>
    LinkedList<T>::LinkedList(): m_head(new Node(0xDEADBEEF, reinterpret_cast<Node *>(0xDEADBEEF))), m_tail(m_head)
    {
        // Do nothing
    }

    template <typename T>
    LinkedList<T>::~LinkedList()
    {
        while (m_head != m_tail)
        {
            auto temp = m_head->m_next;
            delete(m_head);
            m_head = temp;
        }

        delete(m_head);

        m_head = nullptr;
        m_tail = nullptr;
    }


    template <typename T>
    typename LinkedList<T>::Node *LinkedList<T>::Insert(Node *iter_, T data_)
    {
        Node *new_node = new Node(iter_->m_data, iter_->m_next);

        if (IsIterEqual(new_node, m_tail))
        {
            iter_->m_data = data_;
            iter_->m_next = new_node;
            m_tail = new_node;
        }
        else 
        {
            new_node->m_data = data_;
            iter_->m_next = new_node;
            iter_ = new_node;
        }

        return iter_;
    }

    template <typename T>
    bool LinkedList<T>::IsIterEqual(Node *iter1_, Node *iter2_)
    {
        return (iter1_->m_data == iter2_->m_data) && (iter1_->m_next == iter2_->m_next);
    }

    template <typename T>
    T LinkedList<T>::Remove(Node *node)
    {
        Node *temp = node->m_next;

        T data = node->m_data;

        if (node == m_tail)
        {
            throw std::runtime_error("It is impossible to remove a dummy node!");
        }    
        
        node->m_data = node->m_next->m_data;
        node->m_next = node->m_next->m_next;

        if (IsIterEqual(node, m_tail))
        {
            m_tail = node;
        }
        
        delete(temp);

        return data;
    }

    template <typename T>
    std::size_t LinkedList<T>::Size()
    {
        std::size_t size = 0;

        if (IsEmpty())
        {
            return 0;
        }

        Node *temp = m_head;

        while (temp != m_tail && (temp = temp->m_next))
        {
            ++size;
        }

        return size;
    }

    template <typename T>
    bool LinkedList<T>::IsEmpty()
    {
        return m_head == m_tail;
    }

    template <typename T>
    void LinkedList<T>::Clear()
    {
        while (m_head != m_tail)
        {
            auto temp = m_head->m_next;
            delete(m_head);
            m_head = temp;
        }

        m_tail = m_head;
    }

    template <typename T>
    void LinkedList<T>::Find(T data_)
    {

    } 

    template <typename T>
    typename LinkedList<T>::Node *LinkedList<T>::Begin() const
    {
        return m_head;
    }

    template <typename T>
    typename LinkedList<T>::Node *LinkedList<T>::End() const
    {
        return m_tail;
    }

    template <typename T>
    LinkedList<T>::Node::Node(T data_, Node *next_): m_data(data_), m_next(next_)
    {
        // Do nothing
    }

    template <typename T>
    inline T LinkedList<T>::Node::GetData() const
    {
        if (this->m_data == 0xDEADBEEF)
        {
            throw std::runtime_error("This is a dummy node!");
        }
        return m_data;
    }

    template <typename T>
    inline bool LinkedList<T>::Node::HasNext() const
    {
        return nullptr == m_next;
    }

    template <typename T>
    inline typename LinkedList<T>::Node *LinkedList<T>::Node::Next()
    {
        return m_next;
    }
}   // The end of namespace

#endif  // __TD_LINKED_LIST_HPP__
