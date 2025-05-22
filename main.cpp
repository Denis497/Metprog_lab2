#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <map>
#ifdef _WIN32
#include <windows.h>
#endif

/// @brief Структура данных, в которой осуществляется поиск.
///
/// Содержит идентификатор, строковое имя (ключ для поиска) и некоторое значение.
struct Object {
    size_t    id;    ///< Уникальный идентификатор объекта.
    std::string name;///< Имя объекта, по которому производится поиск.
    double    value; ///< Некоторое числовое значение, ассоциированное с объектом.

    /// @brief Конструктор для инициализации всех полей.
    /// @param id_    Идентификатор.
    /// @param name_  Имя.
    /// @param value_ Значение.
    Object(size_t id_, std::string name_, double value_)
            : id(id_), name(std::move(name_)), value(value_) {}
};

/// @brief Глобальный генератор случайных чисел для всего кода.
static std::mt19937_64 rng{ std::random_device{}() };

/// @brief Генерирует случайную строку имени в формате "NameX".
/// @param nameCount Количество различных имен (диапазон X: 0..nameCount-1).
/// @return Строка вида "Name<number>".
std::string generateRandomName(int nameCount) {
    std::uniform_int_distribution<int> dist(0, nameCount - 1);
    return "Name" + std::to_string(dist(rng));
}

/// @brief Генерирует вектор объектов заданного размера с случайными данными.
///
/// Имена объектов выбираются случайно из ограниченного набора для обеспечения дубликатов.
/// @param size Количество элементов, которое необходимо сгенерировать.
/// @return Вектор сгенерированных объектов.
std::vector<Object> generateData(size_t size) {
    std::vector<Object> data;
    data.reserve(size);
    // const int nameCount = 1000;
    size_t nameCount = std::max<size_t>(size / 5, 1);
    std::uniform_real_distribution<double> valDist(0.0, 100.0);

    for (size_t i = 0; i < size; ++i) {
        data.emplace_back(
                i + 1,
                generateRandomName(static_cast<int>(nameCount)),
                valDist(rng)
        );
    }
    return data;
}

/// @brief Линейный поиск всех объектов с заданным именем в массиве.
///
/// Последовательно перебирает каждый элемент и сравнивает поле name.
/// @param data Вектор объектов, в котором выполняется поиск.
/// @param key  Искомое имя (ключ поиска).
/// @return Вектор всех объектов, имя которых равно key.
std::vector<Object> linearSearch(const std::vector<Object>& data, const std::string& key) {
    std::vector<Object> result;
    for (const auto& obj : data) {
        if (obj.name == key) {
            result.push_back(obj);
        }
    }
    return result;
}

/// @brief Класс для реализации невыровненного бинарного дерева поиска (BST) по ключу name.
///
/// Поддерживает хранение нескольких объектов с одинаковым ключом в одном узле.
class BinarySearchTree {
public:
    /// @brief Внутренняя структура узла BST.
    struct Node {
        std::string         key;    ///< Ключевое поле (name).
        std::vector<Object> values; ///< Все объекты с данным ключом.
        Node*               left{nullptr};  ///< Левый потомок.
        Node*               right{nullptr}; ///< Правый потомок.

        /// @brief Конструктор узла.
        /// @param name Ключ.
        /// @param obj  Объект, который добавляется в values.
        Node(const std::string& name, const Object& obj)
                : key(name), values{obj} {}
    };

    BinarySearchTree() = default;
    ~BinarySearchTree() { clear(root); }

    /// @brief Вставляет объект в дерево поиска по его name.
    ///
    /// Если ключ уже есть — добавляет объект в вектор существующего узла.
    /// @param obj Объект для вставки.
    void insert(const Object& obj) {
        if (!root) {
            root = new Node(obj.name, obj);
            return;
        }
        Node* cur = root;
        while (true) {
            if (obj.name == cur->key) {
                cur->values.push_back(obj);
                return;
            }
            if (obj.name < cur->key) {
                if (!cur->left) {
                    cur->left = new Node(obj.name, obj);
                    return;
                }
                cur = cur->left;
            } else {
                if (!cur->right) {
                    cur->right = new Node(obj.name, obj);
                    return;
                }
                cur = cur->right;
            }
        }
    }

    /// @brief Осуществляет поиск всех объектов с заданным именем.
    /// @param key Искомый ключ (name).
    /// @return Вектор найденных объектов (может быть пустым).
    std::vector<Object> search(const std::string& key) const {
        Node* cur = root;
        while (cur) {
            if (key == cur->key) {
                return cur->values;
            }
            if (key < cur->key) {
                cur = cur->left;
            } else {
                cur = cur->right;
            }
        }
        return {};
    }

private:
    Node* root{nullptr}; ///< Корневой узел.

    /// @brief Рекурсивно освобождает память, занимаемую поддеревом.
    /// @param n Корень поддерева.
    void clear(Node* n) {
        if (!n) return;
        clear(n->left);
        clear(n->right);
        delete n;
    }
};

/// @brief Класс красно-черного дерева (Red-Black Tree) для поиска по ключу name.
///
/// Гарантирует балансировку и поиск за O(log n).
class RedBlackTree {
public:
    /// @brief Цвет узла.
    enum Color { RED, BLACK };

    /// @brief Структура узла красно-черного дерева.
    struct Node {
        std::string         key;    ///< Ключ узла (name).
        std::vector<Object> values; ///< Все объекты с данным ключом.
        Color               color;  ///< Цвет узла.
        Node*               left{nullptr};   ///< Левый потомок.
        Node*               right{nullptr};  ///< Правый потомок.
        Node*               parent{nullptr}; ///< Родитель.

        /// @brief Конструктор узла.
        /// @param name Ключ.
        /// @param obj  Объект для values.
        /// @param c    Цвет (RED или BLACK).
        /// @param p    Родительский узел.
        Node(const std::string& name, const Object& obj, Color c, Node* p)
                : key(name), values{obj}, color(c), parent(p) {}
    };

    RedBlackTree() = default;
    ~RedBlackTree() { clear(root); }

    /// @brief Вставляет объект в красно-черное дерево с балансировкой.
    /// @param obj Объект для вставки.
    void insert(const Object& obj) {
        if (!root) {
            root = new Node(obj.name, obj, BLACK, nullptr);
            return;
        }
        Node* cur = root;
        Node* parent = nullptr;
        while (cur) {
            parent = cur;
            if (obj.name == cur->key) {
                cur->values.push_back(obj);
                return;
            }
            cur = (obj.name < cur->key ? cur->left : cur->right);
        }
        Node* node = new Node(obj.name, obj, RED, parent);
        if (obj.name < parent->key) parent->left  = node;
        else                         parent->right = node;
        insertFix(node);
    }

    /// @brief Осуществляет поиск всех объектов с заданным именем.
    /// @param key Искомое имя.
    /// @return Вектор найденных объектов.
    std::vector<Object> search(const std::string& key) const {
        Node* cur = root;
        while (cur) {
            if (key == cur->key) return cur->values;
            if (key < cur->key)  cur = cur->left;
            else                 cur = cur->right;
        }
        return {};
    }

private:
    Node* root{nullptr}; ///< Корень дерева.

    /// @brief Восстанавливает баланс после вставки узла.
    /// @param n Вставленный узел.
    void insertFix(Node* n) {
        while (n != root && n->parent->color == RED) {
            Node* p = n->parent;
            Node* g = p->parent;
            if (p == g->left) {
                Node* u = g->right;
                if (u && u->color == RED) {
                    // Случай 1: перекраска
                    p->color = BLACK;
                    u->color = BLACK;
                    g->color = RED;
                    n = g;
                } else {
                    if (n == p->right) {
                        // Случай 2: левый поворот вокруг p
                        n = p;
                        rotateLeft(n);
                        p = n->parent;
                        g = p->parent;
                    }
                    // Случай 3: правый поворот вокруг g
                    p->color = BLACK;
                    g->color = RED;
                    rotateRight(g);
                }
            } else {
                // Симметричные случаи, когда p — правый ребёнок g
                Node* u = g->left;
                if (u && u->color == RED) {
                    p->color = BLACK;
                    u->color = BLACK;
                    g->color = RED;
                    n = g;
                } else {
                    if (n == p->left) {
                        n = p;
                        rotateRight(n);
                        p = n->parent;
                        g = p->parent;
                    }
                    p->color = BLACK;
                    g->color = RED;
                    rotateLeft(g);
                }
            }
        }
        root->color = BLACK;  // корень всегда черный
    }

    /// @brief Левый поворот поддерева вокруг узла x.
    /// @param x Узел, вокруг которого крутят влево.
    void rotateLeft(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        if (y->left) y->left->parent = x;
        transplant(x, y);
        y->left   = x;
        x->parent = y;
    }

    /// @brief Правый поворот поддерева вокруг узла x.
    /// @param x Узел, вокруг которого крутят вправо.
    void rotateRight(Node* x) {
        Node* y = x->left;
        x->left = y->right;
        if (y->right) y->right->parent = x;
        transplant(x, y);
        y->right  = x;
        x->parent = y;
    }

    /// @brief Вспомогательная замена поддерева u на v.
    /// @param u Узел, который нужно убрать.
    /// @param v Узел, на который заменить.
    void transplant(Node* u, Node* v) {
        if (!u->parent)           root = v;
        else if (u == u->parent->left)  u->parent->left  = v;
        else                            u->parent->right = v;
        if (v) v->parent = u->parent;
    }

    /// @brief Рекурсивно удаляет все узлы.
    /// @param n Корень поддерева.
    void clear(Node* n) {
        if (!n) return;
        clear(n->left);
        clear(n->right);
        delete n;
    }
};

/// @brief Класс хеш-таблицы для поиска по строковому ключу с цепочечным разрешением коллизий.
///
/// Использует полиномиальный роллинг-хеш и вектор бакетов.
class HashTable {
public:
    /// @brief Конструктор хеш-таблицы.
    /// @param tableSize Число бакетов (размер массива бакетов).
    explicit HashTable(size_t tableSize)
            : size(tableSize), buckets(tableSize), collisionCount(0) {}

    /// @brief Вставляет объект в хеш-таблицу.
    ///
    /// Если бакет уже не пуст — это коллизия.
    /// @param obj Объект для вставки.
    void insert(const Object& obj) {
        size_t idx = hashFunction(obj.name);
        if (!buckets[idx].empty()) {
            ++collisionCount;
        }
        buckets[idx].push_back(obj);
    }

    /// @brief Осуществляет поиск всех объектов с заданным именем.
    /// @param key Искомое имя.
    /// @return Вектор найденных объектов.
    std::vector<Object> search(const std::string& key) const {
        size_t idx = hashFunction(key);
        std::vector<Object> result;
        for (const auto& o : buckets[idx]) {
            if (o.name == key) {
                result.push_back(o);
            }
        }
        return result;
    }

    /// @brief Возвращает число коллизий, произошедших при вставке всех элементов.
    /// @return Количество коллизий.
    size_t getCollisionCount() const {
        return collisionCount;
    }

private:
    size_t size;                             ///< Размер хеш-таблицы.
    std::vector<std::vector<Object>> buckets;///< Бакеты с цепочками.
    size_t collisionCount;                   ///< Счетчик коллизий.

    /// @brief Собственная хеш-функция (полиномиальный роллинг-хеш).
    /// @param key Строковый ключ.
    /// @return Индекс бакета [0..size-1].
    size_t hashFunction(const std::string& key) const {
        unsigned long h = 0;
        const unsigned long P = 131;
        for (unsigned char c : key) {
            h = (h * P + c) % size;
        }
        return h;
    }
};

/// @brief Осуществляет поиск всех объектов с заданным именем с помощью std::multimap.
/// @param mmap Стандартный multimap<name, Object>.
/// @param key  Искомое имя.
/// @return Вектор найденных объектов.
std::vector<Object> multimapSearch(const std::multimap<std::string, Object>& mmap,
                                   const std::string& key) {
    std::vector<Object> result;
    auto range = mmap.equal_range(key);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    /// @brief Список размеров массива для тестирования.
    std::vector<size_t> testSizes = {
            100, 50000, 100000,
            200000, 300000, 400000, 500000, 600000, 750000, 1000000
    };

    std::ofstream resultFile("search_results.csv");
    resultFile << "Size,Linear,BST,RBT,Hash,Multimap,Collisions\n";

    std::uniform_int_distribution<size_t> idxDist;

    for (size_t n : testSizes) {
        std::cout << "Генерация данных размера " << n << "...\n";
        auto data = generateData(n);

        BinarySearchTree bst;
        RedBlackTree      rbt;
        HashTable         hashTable(data.size());
        std::multimap<std::string, Object> mmap;
        for (const auto& o : data) {
            bst.insert(o);
            rbt.insert(o);
            hashTable.insert(o);
            mmap.insert({o.name, o});
        }
        size_t collisions = hashTable.getCollisionCount();

        idxDist = std::uniform_int_distribution<size_t>(0, data.size() - 1);

        std::vector<std::string> searchKeys;
        for (int i = 0; i < 10; ++i) {
            searchKeys.push_back(data[idxDist(rng)].name);
        }

        long long sumLin = 0, sumBST = 0, sumRBT = 0, sumHash = 0, sumMM = 0;
        for (const auto& key : searchKeys) {
            auto t0 = std::chrono::high_resolution_clock::now();
            auto r1 = linearSearch(data, key);
            auto t1 = std::chrono::high_resolution_clock::now();
            sumLin += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

            t0 = std::chrono::high_resolution_clock::now();
            auto r2 = bst.search(key);
            t1 = std::chrono::high_resolution_clock::now();
            sumBST += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

            t0 = std::chrono::high_resolution_clock::now();
            auto r3 = rbt.search(key);
            t1 = std::chrono::high_resolution_clock::now();
            sumRBT += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

            t0 = std::chrono::high_resolution_clock::now();
            auto r4 = hashTable.search(key);
            t1 = std::chrono::high_resolution_clock::now();
            sumHash += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

            t0 = std::chrono::high_resolution_clock::now();
            auto r5 = multimapSearch(mmap, key);
            t1 = std::chrono::high_resolution_clock::now();
            sumMM += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        }

        long long avgLin  = sumLin  / static_cast<long long>(searchKeys.size());
        long long avgBST  = sumBST  / static_cast<long long>(searchKeys.size());
        long long avgRBT  = sumRBT  / static_cast<long long>(searchKeys.size());
        long long avgHash = sumHash / static_cast<long long>(searchKeys.size());
        long long avgMM   = sumMM   / static_cast<long long>(searchKeys.size());

        resultFile
                << n << ','
                << avgLin  << ','
                << avgBST  << ','
                << avgRBT  << ','
                << avgHash << ','
                << avgMM   << ','
                << collisions
                << '\n';

        std::cout << "n=" << n
                  << " Lin=" << avgLin
                  << " BST=" << avgBST
                  << " RBT=" << avgRBT
                  << " Hash=" << avgHash
                  << " MM=" << avgMM
                  << " coll=" << collisions
                  << "\n";
    }

    return 0;
}
