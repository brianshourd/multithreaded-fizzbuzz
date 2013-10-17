#include <iostream>
#include <queue>
#include <string>
#include <pthread.h>
using namespace std;

// Goal: Create a fizzbuzz program using threads
// 1 thread for each multiple/word pair (use class Fizz)
//   - each thread will dump to its own queue a string: e.g. either ""
//     or "Fizz"
// 1 printer thread
//   - pops the queues, assembles the words, and prints them out
//   - reactivates each placer if it's queue gets low
//   - if no words, prints current count
// The queues have a min and max size, and when they are full, the word
//   threads wait until they reach the min size

#define NUM_PAIRS 2
#define MIN_Q_SIZE 3
#define MAX_Q_SIZE 10
#define HIGHEST 100

// Fizz class - for word/multiple pair (e.g. 3 and "Fizz")
class Fizz {
    private:
        int multiple;
        string word;
    public:
        Fizz(int multiple_, string word_) {
            multiple = multiple_;
            word = word_;
        }
        int getMultiple() { return multiple; }
        string getWord() { return word; }
};

// Globals
queue<string> queues[NUM_PAIRS];
Fizz* pairs[NUM_PAIRS];
pthread_mutex_t queue_mutex[NUM_PAIRS];
pthread_cond_t queue_min_cv[NUM_PAIRS];
pthread_cond_t queue_max_cv[NUM_PAIRS];

// Thread functions
// placer simply places a word (either the empty word or the word passed
// given it) into the queue until the queue is full, then waits until
// the queue is less full
void *placer(void *t) {
    int id = (int)t;
    Fizz* pair = pairs[id];
    string word = pair->getWord();
    int multiple = pair->getMultiple();
    pthread_mutex_t* mutex = &queue_mutex[id];
    pthread_cond_t* min_cv = &queue_min_cv[id];
    pthread_cond_t* max_cv = &queue_max_cv[id];
    queue<string>* q = &queues[id];
    int i;
    bool shouldSignal;
    for (i = 0; i < HIGHEST; i++) {
        pthread_mutex_lock(mutex);
        if (q->size() >= MAX_Q_SIZE) {
            // Don't need more, wait until we do
            pthread_cond_wait(max_cv, mutex);
        }
        if (i % multiple != 0) {
            q->push("");
        } else {
            q->push(word);
        }
        if (q->size() > MIN_Q_SIZE) {
            // Signal that we can start popping
            shouldSignal = true;
        }
        pthread_mutex_unlock(mutex);
        if (shouldSignal) {
            pthread_cond_signal(min_cv);
            shouldSignal = false;
        }
    }
    pthread_exit(NULL);
}

// printer checks all the queues. If one is too empty, it signals that
// thread to put more in. If not, it pops off the front element in the
// queue and appends it to a string to print. When it has done this, it
// either prints the string or it's current count.
void *printer(void *t) {
    int count = 0;
    int i;
    string out = "";
    queue<string>* q;
    pthread_mutex_t* mutex;
    bool shouldSignal = false;
    while (count <= HIGHEST) {
        out = "";
        for (i = 0; i < NUM_PAIRS; i++) {
            q = &queues[i];
            mutex = &queue_mutex[i];
            pthread_mutex_lock(mutex);
            // Check that we have enough, else wait until we do
            while (q->size() <= MIN_Q_SIZE) {
                pthread_cond_wait(&queue_min_cv[i], mutex);
            }
            // Take one, then signal that we need more, then unlock
            out += q->front();
            q->pop();
            if (q->size() < MAX_Q_SIZE) {
                // Signal that we can add more now
                shouldSignal = true;
            }
            pthread_mutex_unlock(mutex);
            if (shouldSignal) {
                pthread_cond_signal(&queue_max_cv[i]);
                shouldSignal = false;
            }
        }
        if (out == "") {
            cout << count << endl;
        } else {
            cout << out << endl;
        }
        count++;
    }
    pthread_exit(NULL);
}

// Main
int main() {
    pairs[0] = new Fizz(3, "Fizz");
    pairs[1] = new Fizz(5, "Buzz");
    pthread_t threads[NUM_PAIRS + 1];
    //Create threads in joinable state, for portability?
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int i;
    for (i = 0; i < NUM_PAIRS; i++) {
        // Initialize mutexes and conditions
        pthread_mutex_init(&queue_mutex[i], NULL);
        pthread_cond_init(&queue_min_cv[i], NULL);
        pthread_cond_init(&queue_max_cv[i], NULL);
        
        // Create threads
        pthread_create(&threads[i], &attr, placer, (void *)i);
    }
    // Also, create the printer thread
    pthread_create(&threads[NUM_PAIRS], &attr, printer, NULL);

    // Wait for all threads to complete
    for (i = 0; i < NUM_PAIRS + 1; i++) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    pthread_attr_destroy(&attr);
    for (i = 0; i < NUM_PAIRS; i++) {
        pthread_mutex_destroy(&queue_mutex[i]);
        pthread_cond_destroy(&queue_min_cv[i]);
        pthread_cond_destroy(&queue_max_cv[i]);
    }
    pthread_exit(NULL);
}
