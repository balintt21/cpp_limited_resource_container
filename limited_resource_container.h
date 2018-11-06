#ifndef SRC_UTIL_LIMITED_RESOURCE_CONTAINER_H_
#define SRC_UTIL_LIMITED_RESOURCE_CONTAINER_H_

#include <cstddef>
#include <vector>
#include <memory>
#include <type_traits>
#include <functional>

template<typename T, bool IS_SHARED = false>
class LimitedResourceContainer
{
public:
    typedef T                                                                    value_t;
    typedef typename std::conditional<IS_SHARED, std::shared_ptr<T>, std::unique_ptr<T> >::type 
                                                                                 container_value_t;                                                             
    typedef std::vector<container_value_t>                                       container_t;

    using param_type = typename std::conditional<IS_SHARED, const std::shared_ptr<T>&, T* >::type;
private:
    container_t     mResources;
    size_t          mCursor;

public:
    LimitedResourceContainer(size_t limit) : mResources(limit), mCursor(0)
    {}

    virtual ~LimitedResourceContainer() { mResources.clear(); }

    size_t limit() const { return mResources.size(); }

    ssize_t tryAdd(param_type resource)
    {
        ssize_t id = -1;
        if( mResources[mCursor] == nullptr )
        {
            id = mCursor++;
            mResources[id] = container_value_t(resource);
        } else {
            int try_cnt = 2;
            size_t previous_cursor = mCursor;
            while(try_cnt > 0)
            {
                size_t end_cursor = (try_cnt < 2) ? previous_cursor : mResources.size();
                while( (mCursor < end_cursor) && (mResources[mCursor] != nullptr) )
                { ++mCursor; }

                if( mCursor < mResources.size() )
                {
                    id = mCursor++;
                    mResources[id] = container_value_t(resource);
                    try_cnt = 0;
                } else {
                    mCursor = 0;
                    --try_cnt;
                }
            }
        }

        if( mCursor == mResources.size() ) { mCursor = 0; }//!

        return id;
    }

    void remove(ssize_t id)
    {
        if( (id >= 0) && (static_cast<size_t>(id) < mResources.size()) )
        {
            mResources[id].reset();
            mCursor = id;
        }
    }

    container_value_t& get(ssize_t id)
    {
        static container_value_t _default;
        if( (id >= 0) && (static_cast<size_t>(id) < mResources.size()) )
        {
            return mResources[id];
        }
        return _default;
    }

    typename container_t::const_iterator cbegin() const 
    { 
        return mResources.cbegin(); 
    }

    typename container_t::const_iterator cend() const 
    { 
        return mResources.cend(); 
    }

    void foreach(const std::function<bool (container_value_t&)>& yield)
    {
        if( yield != nullptr )
        {
            for( auto& resource : mResources )
            {
                if( !yield(resource) ) {  break; }
            }
        }
    }

    void foreach(const std::function<bool (value_t&)>& yield)
    {
        if( yield != nullptr )
        {
            for( auto& resource : mResources )
            {
                if( (resource != nullptr) && !yield(*resource) ) {  break; }
            }
        }
    }
};


#endif /* SRC_UTIL_LIMITED_RESOURCE_CONTAINER_H_ */
