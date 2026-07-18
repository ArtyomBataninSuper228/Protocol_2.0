//
//  access_control.hpp
//  MyProtocolCore
//
//  Created by Артем Батанин on 17.07.2026.
//

#ifndef access_control_
#define access_control_
#pragma once

#include "stuff.hpp"
#include <unordered_set>


#pragma GCC visibility push(default)

template <typename Encoder>
class Base_Access_Controller{
    using Key = std::array<uint8_t, Encoder::PUBLIC_KEY_SIZE>;
    
    struct KeyHash{
        Crc32Hasher hasher = Crc32Hasher();
        size_t operator()(const Key& k) const noexcept{
            return hasher.operator()(k);
        }
    };
    
    std::atomic<std::shared_ptr<const std::unordered_set<Key, KeyHash>>> snapshot;
    std::mutex write_mutex;
    using Snapshot = std::unordered_set<Key, KeyHash>;
    
public:
    bool is_allowed(const Key& key) const noexcept{
        auto snap = snapshot.load(std::memory_order_acquire);
        return snap->contains(key);
    }
    void replace_all(std::unordered_set<Key, KeyHash> new_set){
        snapshot.store(std::make_shared<const std::unordered_set<Key, KeyHash>>(std::move(new_set)), std::memory_order_release);
    }
    
    void add(const Key& key){
        std::lock_guard<std::mutex> lock (write_mutex);
        auto old_snap = snapshot.load(std::memory_order_acquire);
        auto new_snap = std::make_shared<Snapshot>(*old_snap);
        new_snap->insert(key);
        snapshot.store(std::move(new_snap), std::memory_order_release);
    }
    
    void revoke(const Key& key){
        std::lock_guard<std::mutex> lock (write_mutex);
        auto old_snap = snapshot.load(std::memory_order_acquire);
        auto new_snap = std::make_shared<Snapshot>(*old_snap);
        new_snap->erase(key);
        snapshot.store(std::move(new_snap), std::memory_order_release);
        
    }
    
};

template <typename Encoder>
class Alvays_True_Access_Control{
    using Key = std::array<uint8_t, Encoder::PUBLIC_KEY_SIZE>;
public:
    bool is_allowed(const Key& key) const noexcept{
        return true;
    }
    
};


#pragma GCC visibility pop
#endif
