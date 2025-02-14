#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

 
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock, [this] {return !_queue.empty();});
    T msg = std::move(_queue.front());
    _queue.pop_front();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.push_back(std::move(msg));
    _condition.notify_one();
}


/* Implementation of class "TrafficLight" */

 
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        if (_messageQueue.receive() == TrafficLightPhase::green){
            return;
        }
    }
}


TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard<std::mutex> lock(_phaseMutex);
    return _currentPhase;
}

void TrafficLight::setCurrentPhase(TrafficLightPhase phase)
{
    std::lock_guard<std::mutex> lock(_phaseMutex);
    _currentPhase = phase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    
    // launch cycleThroughPhases function in a thread and add it to the thread queue)
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    
    // print id of the current thread
    std::unique_lock<std::mutex> lck(_mtx);
    std::cout << "TrafficLight #" << _id << "::cycleThroughPhases: thread id = " << std::this_thread::get_id() << std::endl;
    lck.unlock();

    // random duration of a single simulation cycle: between 4 and 6 seconds
    std::random_device rd; // obtain a seed for the random number engine.
    std::mt19937 gen(rd()); // random number engine seeded with rd()
    std::uniform_int_distribution<> dis(4000, 6000); // Uniform distribution between 4000 and 6000 milliseconds (corresponds to 4 and 6 seconds)
    double cycleDuration = dis(gen); // Generate initial random duration
    
    // init stop watch
    auto lastUpdate = std::chrono::system_clock::now();
    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        if (timeSinceLastUpdate >= cycleDuration)
        {
            // toggle the current phase of the traffic light between red and green
            if (getCurrentPhase()==TrafficLightPhase::red)
            {
                setCurrentPhase(TrafficLightPhase::green);
            }
            else if (getCurrentPhase()==TrafficLightPhase::green)
            {
                setCurrentPhase(TrafficLightPhase::red);
            }
            // send an update method to the message queue using move semantics.
            _messageQueue.send(std::move(_currentPhase));
            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();
        }
    }
}

