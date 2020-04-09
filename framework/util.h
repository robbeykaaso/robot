#ifndef REAL_FRAMEWORK_UTIL_H_
#define REAL_FRAMEWORK_UTIL_H_
#include <string>
#include <QMap>
#include <QSet>
#include <QHostInfo>
//#include <QList>
#include <stack>
#include <iostream>

#ifdef EXPORTDLL
#define DSTDLL __declspec(dllexport)
#else
#define DSTDLL __declspec(dllimport)
#endif

namespace dst {

DSTDLL QString getCWD(const char* aSuffix);
DSTDLL QString generateUUID();
DSTDLL QString GetMachineFingerPrint();
DSTDLL void tic();
//https://blog.csdn.net/Wangguang_/article/details/93880452
DSTDLL int getRandom(int min,int max);
DSTDLL QHostAddress GetLocalIP();
DSTDLL QString getWMIC(const QString &cmd);
DSTDLL QString int2Hex(int aInt);
DSTDLL QStringList parseJsons(const QString& aContent);

template <typename K, typename T>
T* tryFind(QMap<K, T>* aMap, const K& aKey){
    auto ret = aMap->find(aKey);
    if (ret == aMap->end()){
        aMap->insert(aKey, T());
        ret = aMap->find(aKey);
    }
    return &(*ret);
}

//copy from: tinny-dnn
class DSTDLL progress_display {
public:
    explicit progress_display(size_t expected_count_,
                              std::ostream &os      = std::cout,
                              const std::string &s1 = "\n",  // leading strings
                              const std::string &s2 = "",
                              const std::string &s3 = "")
        // os is hint; implementation may ignore, particularly in embedded systems
        : m_os(os), m_s1(s1), m_s2(s2), m_s3(s3) {
        restart(expected_count_);
    }

    void restart(size_t expected_count_);

    size_t operator+=(size_t increment) {
        //  Effects: Display appropriate progress tic if needed.
        //  Postconditions: count()== original count() + increment
        //  Returns: count().
        if ((_count += increment) >= _next_tic_count) {
            display_tic();
        }
        return _count;
    }

    size_t operator++() { return operator+=(1); }
    size_t count() const { return _count; }
    size_t expected_count() const { return _expected_count; }

private:
    std::ostream &m_os;      // may not be present in all imps
    const std::string m_s1;  // string is more general, safer than
    const std::string m_s2;  //  const char *, and efficiency or size are
    const std::string m_s3;  //  not issues

    size_t _count, _expected_count, _next_tic_count;
    size_t _tic;
    void display_tic();

    progress_display &operator=(const progress_display &) = delete;
};

//https://www.geeksforgeeks.org/topological-sorting/
template <typename T>
class DSTDLL topoSort
{
public:
    void addEdge(T aPrevious, T aNext){
        tryFind(&m_edges, aNext)->push_back(aPrevious);
    }
    void sort(QList<T>& aResult){
        for (auto i : m_edges.keys())
            doSort(i, aResult);
    }
private:
    void doSort(T aNode, QList<T>& aResult){
        if (!m_visited.contains(aNode)){
            m_visited.insert(aNode);
            auto previous = m_edges.value(aNode);
            for (auto i : previous)
                doSort(i, aResult);
            aResult.push_back(aNode);
        }
    }
    QMap<T, QList<T>> m_edges;
    QSet<T> m_visited;
};

DSTDLL void showDstLog(const QString& aMessage, const QString& aLevel = "info");

}

#endif
