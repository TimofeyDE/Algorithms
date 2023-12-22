#include <iostream> // std::cout, std::cin, std::endl
#include <string>   // std::string
#include <cstddef> // std::size_t

#include "LinkedList.hpp"   // LinkedList
                           
int g_total = 0;
int g_success = 0;

const std::string RED = "\033[0m\n";
const std::string RESET = "\x1B[0m";
const std::string GREEN = "\x1B[32m";

using namespace TD;

template <typename A, typename B>
void Test(std::string message, A expected, B actual, int line);
void Pass();

void TestManagementFunctions();

int main()
{
    TestManagementFunctions();
    Pass();
    return 0;
}

void TestManagementFunctions()
{
    int arr_1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    LinkedList<int> list;

    Test("Size", list.Size(), 0, __LINE__);
    Test("IsEmpty", list.IsEmpty(), 1, __LINE__);
    Test("Begin == End", list.IsIterEqual(list.Begin(), list.End()), 1, __LINE__);

    for (auto & item : arr_1)
    {
        list.Insert(list.Begin(), item);
    }

    Test("Size", list.Size(), 10, __LINE__);
    Test("IsEmpty", list.IsEmpty(), 0, __LINE__);
    Test("Begin == End", list.IsIterEqual(list.Begin(), list.End()), false, __LINE__);

    std::size_t idx = 9;
    for (auto node = list.Begin(); node != list.End(); node = node->Next())
    {
        Test("Element", arr_1[idx--], node->GetData(), __LINE__);
    }

    list.Clear();

    Test("Size", list.Size(), 0, __LINE__);
    Test("IsEmpty", list.IsEmpty(), 1, __LINE__);
    Test("Begin == End", list.IsIterEqual(list.Begin(), list.End()), true, __LINE__);

    for (auto & item : arr_1)
    {
        list.Insert(list.End(), item);
    }

    Test("Size", list.Size(), 10, __LINE__);
    Test("IsEmpty", list.IsEmpty(), 0, __LINE__);
    Test("Begin == End", list.IsIterEqual(list.Begin(), list.End()), false, __LINE__);

    idx = 0;
    for (auto node = list.Begin(); node != list.End(); node = node->Next())
    {
        Test("Element", arr_1[idx++], node->GetData(), __LINE__);
    }

    while (!list.IsEmpty())
    {
        list.Remove(list.Begin());
    }

    Test("Size", list.Size(), 0, __LINE__);
    Test("IsEmpty", list.IsEmpty(), 1, __LINE__);
    Test("Begin == End", list.IsIterEqual(list.Begin(), list.End()), true, __LINE__);
}

template <typename A, typename B>
void Test(std::string message, A expected, B actual, int line)
{
    ++g_total;

    if (expected == actual)
    {
        ++g_success;
    }
    else
    {
        std::cerr << "line: " << line << " " << "expected: " << expected << " " << "actual: " << actual << std::endl;
    }
}

void Pass()
{
    if (g_success == g_total)
    {
        std::cout << GREEN << "Tests passed!" << RESET << std::endl;
    }
    else
    {
        std::cerr << RED << "Tests failed!" << RESET << std::endl;
    }
}
