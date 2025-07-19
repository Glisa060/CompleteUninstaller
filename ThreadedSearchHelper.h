#pragma once

#include <functional>
#include <thread>


// Template funkcija koja izvrsava zadatak u pozadini i rezultat vraca nazad u GUI thread
template<typename ResultType, typename TaskFunc, typename CallbackFunc>
void RunInBackground(TaskFunc task, CallbackFunc callback) {
    std::thread([task = std::move(task), callback = std::move(callback)]() mutable {
        ResultType result = task();
        wxTheApp->CallAfter([callback = std::move(callback), result = std::move(result)]() {
            callback(result);
        });
    }).detach();
}
