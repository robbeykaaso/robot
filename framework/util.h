#ifndef REAL_FRAMEWORK_UTIL_H_
#define REAL_FRAMEWORK_UTIL_H_
#include <string>
#include <QMap>
#include <QSet>
#include <QHostInfo>
//#include <QList>
#include <stack>

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
