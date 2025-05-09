#ifndef ASSETMANAGER_H_INCLUDED
#define ASSETMANAGER_H_INCLUDED

#include "core/core.h"
#include "Asset.h"
#include "Singleton.h"
#include "SpinLock.h"
#include "AddYourLoader.h"

namespace tim
{
    using namespace core;
namespace resource
{
    template<class T>
    class AssetManager : public Singleton<AssetManager<T>>
    {
        friend class Singleton<AssetManager<T>>;

    public:
        template <bool async, typename... Args>
        Option<T> load(Args... args)
        {
            static StaticLoader<Args...> in_loader;

			Option<T> opt = in_loader.get(args...);
            if(opt) return opt;
            else
            {
#ifndef USE_VCPP
				Option<T> opt_dat = in_loader.template operator()<async>(args...);
#else
				Option<T> opt_dat = in_loader.operator()<async>(args...);
#endif

                if(opt_dat)
                    in_loader.add(args..., opt_dat.value());
                return opt_dat;
            }
        }

        virtual ~AssetManager() { clear(); }

        uint nbLoaders() const { return _loaders.size(); }

        void clear()
        {
            std::for_each(_loaders.begin(), _loaders.end(), [](GenericLoader* l) { l->clear(); });
        }

    private:
        class GenericLoader
        {
        public:
            GenericLoader() { AssetManager::instance().addLoaders(this); }
            virtual ~GenericLoader() = default;

            virtual void clear() = 0;
        };

        vector<GenericLoader*> _loaders;
        SpinLock _lock;

        void addLoaders(GenericLoader* l)
        {
            std::lock_guard<SpinLock> guard(_lock);
            _loaders.push_back(l);
        }

        template <typename... Args>
        class Loader : GenericLoader
        {
        public:
            Loader() : GenericLoader() {}
            virtual ~Loader() { clear(); }

            Option<T> get(Args... args) const
            {
                std::lock_guard<SpinLock> guard(_lock);
                auto it = _assets.find(std::make_tuple(args...));
                if(it == _assets.end()) return Option<T>();
                else return Option<T>(it->second);
            }

            void add(Args... args, const T& asset)
            {
                std::lock_guard<SpinLock> guard(_lock);
                _assets[std::make_tuple(args...)] = asset;
            }

            void clear() override
            {
                 std::lock_guard<SpinLock> guard(_lock);
                _assets.clear();
            }

            //typename std::map<std::tuple<Args...>, T>::const_iterator begin() const { return _assets.begin(); }
            //typename std::map<std::tuple<Args...>, T>::const_iterator end() const { return _assets.end(); }

            std::map<std::tuple<Args...>, T> _assets;
            mutable SpinLock _lock;
        };

		template <typename... Args>
		class StaticLoader : public Loader<Args...>, public AssetLoader<T>
		{
		public:
			StaticLoader() : Loader<Args...>(), AssetLoader<T>() {}
		};
    };


}
}

#endif // ASSETMANAGER_H_INCLUDED
