#include <iostream>
#include <queue>
#include <list>
#include <string>
#include <pthread.h>
using namespace std;

// Goal: Create a fizzbuzz program using threads
// 1 thread for each multiple/word pair (use class FizzQueue)
//   - each thread will dump to its own queue a series of strings: e.g.
//     either "" or "Fizz"
// 1 printer thread
//   - pops the queues, assembles the words, and prints them out
//   - waits if a queue is too empty
//   - if no words, prints current count
// The queues have a min and max size, and when they are full, the word
//   threads wait until they aren't

#define MIN_Q_SIZE 3
#define MAX_Q_SIZE 10

// Fizz class - for word/multiple pair (e.g. 3 and "Fizz")
// This class creates and handles its own thread. That is, instances of
// this class run in a thread of their own making, which is cleanly
// destroyed when they are.
// Public methods can safely be called by other threads
// Private methods are called by own thread
class FizzQueue {
    private:
        int multiple;   // e.g. 3
        string word;    // e.g. "Fizz"
        int count;      // Current number

        // Here's the flag to finish the thread
        bool finished;
        
        // The underlying queue, where we will store words, and its
        // mutexes and conditions
        queue<string> words;
        pthread_mutex_t mutexQueue;
        pthread_cond_t condFull;
        pthread_cond_t condEmpty;

        pthread_t tid;
        
        // callFillQueue is basically a hack to call fillQueue from
        // pthread_create, which is impossible for non-static methods
        static void* callFillQueue(void* arg) {
            return ((FizzQueue*)arg)->fillQueue();
        }
        void* fillQueue() {
            bool shouldSignal = false;
            while(!finished) {
                pthread_mutex_lock(&mutexQueue);
                while ((words.size() >= MAX_Q_SIZE) && !finished) {
                    // Don't need more, wait until we do
                    pthread_cond_wait(&condFull, &mutexQueue);
                }
                if (count % multiple != 0) {
                    words.push("");
                } else {
                    words.push(word);
                }
                if (words.size() > MIN_Q_SIZE) {
                    // Signal that we can start popping
                    shouldSignal = true;
                }
                pthread_mutex_unlock(&mutexQueue);
                if (shouldSignal) {
                    pthread_cond_signal(&condEmpty);
                    shouldSignal = false;
                }
                count++;
            }
            cout << "Terminated!" << endl;
            pthread_exit(NULL);
        }
    public:
        FizzQueue(int m, string w) : multiple(m), word(w) {
            count = 0;
            finished = false;

            // Create and initialize mutex things
            //pthread_mutex_init(&mutexFinished, NULL);
            //pthread_cond_init(&condFinished, NULL);
            pthread_mutex_init(&mutexQueue, NULL);
            pthread_cond_init(&condFull, NULL);
            pthread_cond_init(&condEmpty, NULL);

            // Create the thread
            pthread_create(&tid, NULL, FizzQueue::callFillQueue, this);
        }
        ~FizzQueue() {
            // Mark finished
            cout << "Destroying queue with multiple " << multiple << "... ";
            finished = true;
            // Send the signals that we could potentially be waiting on
            pthread_cond_signal(&condFull);
            pthread_cond_signal(&condEmpty);
            // Wait for shutdown
            pthread_join(tid, NULL);

            // Destroy mutex things cleanly
            pthread_mutex_destroy(&mutexQueue);
            pthread_cond_destroy(&condFull);
            pthread_cond_destroy(&condEmpty);
        }

        string pop() {
            bool shouldSignal = false;
            pthread_mutex_lock(&mutexQueue);
            // Check that we have enough, else wait until we do
            while ((words.size() <= MIN_Q_SIZE) && !finished) {
                pthread_cond_wait(&condEmpty, &mutexQueue);
            }
            // Take one, then signal that we need more, then unlock
            string out = words.front();
            words.pop();
            if (words.size() < MAX_Q_SIZE) {
                // Signal that we can add more now
                shouldSignal = true;
            }
            pthread_mutex_unlock(&mutexQueue);
            if (shouldSignal) {
                pthread_cond_signal(&condFull);
                shouldSignal = false;
            }
            return out;
        }
};

// Printer manages its own threads, creating a thread for every new
// pair, and destroying them after work is done
class Printer {
    private:
        list<FizzQueue*> qs;
        const int max;
        int count;
        void startOver() {
            while (!qs.empty()) {
                delete qs.front();
                qs.pop_front();
            }
            count = 0;
        }
    public:
        Printer(int max_) : max(max_) {
            count = 0;
        }
        ~Printer() {
            startOver();
        }
        void addPair(int multiple, string word) {
            qs.push_back(new FizzQueue(multiple, word));
        }
        void execute() {
            string out;
            while (count <= max) {
                out = "";
                for (auto cur : qs) {
                    out += cur->pop();
                }
                if (out == "") {
                    cout << count << endl;
                } else {
                    cout << out << endl;
                }
                count++;
            }
            startOver();
        }
};

int main() {
    Printer p(100);
    p.addPair(3, "Fizz");
    p.addPair(5, "Buzz");
    p.execute();

    p.addPair(7, "Baz");
    p.execute();
    pthread_exit(NULL);
}
