#ifndef REAL_FRAMEWORK_TASKSEQUENCE_H_
#define REAL_FRAMEWORK_TASKSEQUENCE_H_

#include <QJsonObject>
#include <QJsonArray>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <condition_variable>
#include <mutex>

namespace dst{

class taskSeqs;

class taskSeq{
public:
    taskSeq(taskSeqs* aTaskSeqs, QJsonArray& aTasks,
            const std::map<std::string, std::function<void(const QJsonObject&)>>& aActions, int aTimes = 1) {
        for (auto i : aTasks)
            m_tasks.push_back(i.toObject());
        m_times = aTimes;
        m_whole_times = aTimes;
        for (auto i : aActions)
            m_actions.insert(std::pair<std::string, std::function<void(const QJsonObject&)>>(i.first, i.second));
        m_taskSeqs = aTaskSeqs;
    }
    ~taskSeq(){
        //int test = 0;
    }
    void tryTrigCurrentTask(const QJsonObject& aTrigging, const QJsonObject& aIOConfig);
private:
    std::vector<QJsonObject> m_tasks;
    int m_current = 0;
    int m_times;
    int m_whole_times;
    std::map<std::string, std::function<void(const QJsonObject&)>> m_actions;
    taskSeqs* m_taskSeqs;
};

class taskSeqs{
public:
    void addTaskSeq(std::shared_ptr<taskSeq> aTaskSeq);
    void startSeqs(const QJsonObject& aConfig = QJsonObject());
    bool stopped() {return m_task_stop;}
    void stop(){
        m_task_stop = true;
        m_trigging_signal.notify_one();
    }
    void trigOnce(const QJsonObject& aConfig = QJsonObject()){
        {
            std::lock_guard<std::mutex> lock(m_trigging_mutex);
            m_triggings.push_back(QJsonObject(aConfig));
        }
        m_trigging_signal.notify_one();
    }
private:
    std::vector<std::shared_ptr<taskSeq>> m_task_seqs;
    std::vector<QJsonObject> m_triggings;
    std::mutex m_trigging_mutex;
    std::condition_variable m_trigging_signal;
    bool m_task_stop;
};

}

#endif
