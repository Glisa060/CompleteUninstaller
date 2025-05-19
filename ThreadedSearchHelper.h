#pragma once

#include <functional>
#include <thread>


// Template funkcija koja izvrsava zadatak u pozadini i rezultat vraca nazad u GUI thread
template<typename ResultType>
void RunInBackground(std::function<ResultType()> task, std::function<void(ResultType)> onDone)
{
    std::thread([task, onDone]() {
        ResultType result = task();
        wxTheApp->CallAfter([result, onDone]() {
            onDone(result);
        });
    }).detach();
}
