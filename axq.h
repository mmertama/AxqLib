#ifndef AXQ_H
#define AXQ_H

#include <memory>
#include <functional>
#include <tuple>

#include <QIODevice>
#include <QVariant>

/**
 *  Axq
 * =====
 * Functional Reactive Qt framework
 * --------------------------------
 *
 * Axq provides Functional Reactive programming approach with Qt. Axq implements set of Providers,
 * classes that generate data into Stream. Stream is a way to handle and manipulate data using
 * Operators. Stream ends to error or complete handler.
 *
 * ```
 * Producer->Operator->Operator->Operator->Complete_Handler.
 * ```
 * There are different kind of Producers, e.g. Range producers increasing integers in given range of values. When
 * end of range is reached the given complete handler function is called. Operators are given functions that does
 * a certain operator for data received from stream. For example Map operator transforms from received data to another
 * value into Stream.
 *
 * QML example to generate ASCII values:
 * ```
    function showAscii() {
       lines.text = ""
       Axq.range(0, 255, 1).filter(function(x){
                        return x > 0x20 && x < 0x7E
                    })
        .map(function(x){
                 return  x.toString() + ' ' + String.fromCharCode(x) + (x % 5 ? "\t" : '\n')
            })
        .each(function(acc){
                 lines.text += acc
             })
    }
 * ```
 * Here, in my test code and examples I mostly use chained notation as that apt well with idea of Streams, however
 * nothing prevents code written in more imperative manner.
 *
 * ```
    Axq::Range(0, 255, 1).filter(...
    ```
 * vs.
 * ```
auto producer = Axq::Range(0, 255, 1);
auto filtered = producer.filter(...
    ```
 *
 * The are quite equal APIs for both C++ and Javascript for QML. Streams are mostly asynchronous within created thread,
 * however C++ API provides operators use multiple threads with Stream.
 *
 * Besides just operators there are a good collection of functions to manage the Stream. C++ memory managed is "automated",
 * so that Stream can just be created and then it will deleted some time after completion. For C++ there are operators `own`
 * and `take` to take dynamically created items ownership into Stream and thus released when Stream is released.
 *
 * @toc
 *
 *
 *  ###### *Axq is published under GNU LESSER GENERAL PUBLIC LICENSE, Version 3*
 *  @style date ###### *Copyright Markus Mertama 2018*, generated at %1
 *  @date
 *  _____
 */

#if __cplusplus <= 201402L
template< bool B, class T = void >
using enable_if_t = typename std::enable_if<B, T>::type;
template< class T >
using remove_pointer_t = typename std::remove_pointer<T>::type;
#else
using enable_if_t = std::enable_if_t;
using remove_pointer_t = std::remove_pointer_t;
#endif

/// @cond
#if defined(AXQSHAREDLIB_LIBRARY)
#  define AXQSHAREDLIB_EXPORT Q_DECL_EXPORT
#else
#  define AXQSHAREDLIB_EXPORT Q_DECL_IMPORT
#endif
/// @endcond

/// @cond
#if defined(MEASURE_STREAMS) || defined(MEASURE_TIME)
namespace Axq {
class AXQSHAREDLIB_EXPORT Measure {
public:
#ifdef MEASURE_STREAMS
    static void push();
    static void alloc(QObject* obj);
    static void free(QObject* obj);
    static void pop();
#endif
#ifdef MEASURE_TIME
    static void elapsed(const QString& name);
#endif
};
}
#endif
#ifdef MEASURE_STREAMS
#define STREAM_START_MEM Axq::Measure::push();
#define STREAM_CHECK_MEM Axq::Measure::pop();
#define STREAM_ALLOC(name) Axq::Measure::alloc(name);
#define STREAM_FREE(name) Axq::Measure::free(name);
#else
#define STREAM_START_MEM
#define STREAM_CHECK_MEM
#define STREAM_ALLOC(name)
#define STREAM_FREE(name)
#endif

#ifdef MEASURE_TIME
#define STREAM_ELAPSED(name) Axq::Measure::elapsed(name);
#else
#define STREAM_ELAPSED(name)
#endif
/// @endcond

/**
 * @namespace Axq
 * Axq is using *Axq* namespace.
 *
 * ____________________________
 */

/// @cond

namespace Axq {
Q_NAMESPACE
/// @endcond

class StreamBase;
class ProducerBase;

/**
 * @class ParamList
 * @brief When there is need for multiple input-output parameters a Axq::Params utility will help.
 * Generic parameter container to use multiple parameter within monadic functions.
 *
 */
using ParamList = QList<QVariant>;

/**
 * @class Param
 * Generic parameter to use with ParamList container
 *
 */
using Param = QVariant;

/**
 * @scopeend
 */

/// @cond
template <typename ...Args>
void _toList(QList<QVariant>& lst) {
    Q_UNUSED(lst);
}


template <typename ...Args>
void _toList(ParamList& lst, const QVariant& val) {
    lst.append(val);
}

template <typename ...Args>
void _toList(ParamList& lst, const QVariant& val, Args...args) {
    lst.append(val);
    _toList(lst, args...);
}

template<int pos, typename T>
void _getValue(const ParamList& list, T& value) {
    value = list.at(pos).value<T>();
}

template<int pos, typename T, typename ...Args>
void _getValue(const ParamList& list, T& value, Args& ... args) {
    Q_ASSERT(pos >= 0 && pos < list.length());
    value = list.at(pos).value<T>();
    _getValue < pos + 1, Args... > (list, args...);
}
/// @endcond

template <typename ...Args>
/**
 * @function tie
 * Apply list values to arguments
 * @param ParamList object to read from
 * @param list of out parameters to read values to
 *
 *
 * Example:
 * ```
 *       stream.each<Axq::ParamList>([this](const Axq::ParamList& pair){
 *       QString key; int count;
 *       Axq::tie(pair, key, count);
 *       print("\"", key, "\" exists ", count, " times\n");
 *```
 *
 */
void tie(const ParamList& list, Args& ...args) {
    _getValue<0, Args...>(list, args...);
}

template <int index, typename T>
/**
 * @function get
 * Get a nth parameter from a list
 * @templateparam index of parameter
 * @templateparam type of parameter
 * @param ParamList object to read from
 * @return value
 *
 */
T get(const ParamList& list) {
    T v;
    _getValue<index, T>(list, v);
    return v;
}

/**
 *  @class Stream
 *
 *  Handle to Stream. The Stream is created by producer functions, e.g. `from` and `create`
 *  and then operators like `each` and `map` are applied on it to manage and control the data.
 *  The producer functions and operators returns a Stream object so calls can be chained as
 *  done in the test applications and examples, but naturally there is no reason just call
 *  them individually.
 *
 *  For example two examples below are quite equal.
 * ```
    auto rep = Axq::range(1, 10).each<int>([](int v){...}).map<int, int>([](int v){...});
 * ```
 * and
 * ```
    auto rep = Axq::range(1, 10);
    rep.each<int>([](int v){...});
    rep.map<int, int>([](int v){...});
 *  ```
 *
 */
class Stream;
class StreamPrivate;

//template <class T, typename = std::enable_if<std::is_base_of<Stream, T>::value>>T async(const T& stream);

/// @cond
#define DECLARE_ENUMTYPE(xx)  enum class xx{xx = static_cast<int>(InfoValues::xx)};
/// @endcond

class AXQSHAREDLIB_EXPORT Stream {
private:
    class AXQSHAREDLIB_EXPORT Keeper {
    public:
        Keeper() = default;
        Keeper(const Keeper&) = default;
        virtual ~Keeper();
    };
    class AXQSHAREDLIB_EXPORT Waiter {
    public:
        Waiter() = default;
        Waiter(const Waiter&) = default;
        virtual void finished() = 0;
        virtual ~Waiter();
    };
public:
#ifndef ulong
    using ulong = unsigned long;
#endif
    enum {
        RequestOne = -1
    };

    enum {
        OneLine = -1, Slurp = 0
    };
    static constexpr char StreamCancel[] = "Stream::Cancel";
    static constexpr int StreamCancelValue = 1000;
    enum class InfoValues {This, Index};
    DECLARE_ENUMTYPE(Index)
    DECLARE_ENUMTYPE(This)
public:
    using Streams = std::vector<std::shared_ptr<StreamBase>>;
    template<typename T>
    /**
     * @function each
     * @templateparam stream type
     * @param onEach function, F(value)->void
     * @return Stream
     *
     * Each does nothing to Stream, but a given function is called for each output.
     *
     */
    Stream each(std::function<void (const T&)> onEach) {
        return createEach([onEach](const QVariant & v) {
            onEach(convert<T>(v));
        });
    }

    template<typename O, typename I>
    /**
     * @function map
     * @templateparam mapped stream type out
     * @templateparam stream type in
     * @param onMap function, F(value)->value
     * @return Stream
     *
     * Applies Stream a function that defines the mapping from input to output.
     *
     */
    Stream map(std::function< O(const I&) > onMap) {
        return createMap([onMap](const QVariant & v)->QVariant{
            return convertFrom<O>(onMap(convert<I>(v)));
        });
    }

    /**
     * @function async
     * @return Stream
     *
     * Moves this Stream in its own thread.
     *
     * There is no QML implementation of this function.
     *
     */
    Stream async();

    template <typename... Params, typename... Args>
    /**
     * @function async
     * @param pointer to operator
     * @param argument list of operator parameters
     * @return Stream
     *s
     * Excetes a given Stream Operator in its own thread.
     *
     * Example:
     * ```c++
     * async(&Axq::Stream::each<QString>, [](const QString& out){
     *     ...this is excuted in own thread...
     * });
     * ```
     *
     * There is no QML implementation of this function
     *
     */
    Stream async(Stream(Stream::*f)(Params...), Args&& ... args) {
        auto async = createAsync();
        auto child = (std::get<1>(async).*f)(args...); // 0 owns 1 and then created one
        std::get<1>(async).setChild(child);
        return std::get<0>(async);
    }

    template <typename... Params, typename... Args>
    /**
     * @function split
     * @param pointer to operator
     * @param argument list of operator parameters
     * @return Stream
     *
     * Split current thread input to original and one from given Operator with given parameters applied.
     * The output value is a Axq::Parameters where the first parameter is the original and second output from the
     * Operator's.
     *
     * For QML the name of the function is given as a string.
     *
     */
    Stream split(Stream(Stream::*f)(Params...), Args&& ... args) {
        auto streams = createSplit();
        auto child = (std::get<1>(streams).*f)(args...);
        std::get<0>(streams).setChild(child);
        return std::get<0>(streams);
    }

    template <typename T>
    /**
     * @function filter
     * @templateparam type
     * @param onFilter function, F(value)->boolean
     * @return Stream
     *
     * Applies stream function that removes input from output if returns false
     *
     */
    Stream filter(std::function<bool (const T&)> onFilter) {
        return createFilter([onFilter](const QVariant & v) {
            return onFilter(convert<T>(v));
        });
    }

    template< typename T>
    /**
     * @function waitComplete
     * @templateparam type
     * @param onWait function, F(value)->Stream
     * @return Stream
     *
     * The parameter function is expected a return a nested stream. The outter
     * Stream will not be completed until a given stream is completed. Please note that Axq
     * is asynchronous and therefore the outter producer is not stopped or deferred.
     *
     */
    Stream waitComplete(std::function<Stream(const T&)> onWait) {
        return createSpawn([onWait](const QVariant & v) {
            return onWait(convert<T>(v));
        });
    }

    template <typename T>
    /**
     * @function completeEach
     * @templateparam type
     * @param onPass function, F(value)->boolean
     * @return Stream
     *
     * Does complete the Stream when a given function returns true.
     *
     */
    Stream completeEach(std::function<bool (const T&)> onPass) {
        return createCompleteFilter([onPass](const QVariant & v) {
            return onPass(convert<T>(v));
        });
        return *this;
    }

    /**
     * @function completeEach
     * @return
     *
     * Complete the Stream unconditionally
     *
     */
    Stream completeEach();

    /**
     * @function complete
     * Imperative complete the Stream.
     *
     */
    void complete();


    template<class O, typename F>
    /**
     * @function wait
     * @param sender object
     * @param Qt signal
     * @return Sender
     *
     * Prevents the Stream not to be completed until the given signal is emitted.
     */
    Stream wait(O* sender,  F signal) {
        auto waiterTuple = createWait();
        Stream stream = std::get<0>(waiterTuple);
        Waiter* waiter = std::get<1>(waiterTuple);
        std::tie(stream, waiter) = waiterTuple;
        QObject::connect(sender, signal, [waiter]() {
            waiter->finished();
        });
        return stream;
    }


    template <typename S, typename T>
    /**
     * @function scan
     * @templateparam stream type
     * @param begin initial value
     * @param onScan function, the first parameter is the accumulated value, F(reference to accumulator, value)->void
     * @return Stream
     *
     * The input is collected into accumulator and then output when parent producer is completed.
     *
     * C++ Example:
     * ```c++
     *stream.scan<int>(0, [](int acc, int value){ acc += value;});
     * ```
     *
     * QML Example:
     * ```js
     * Axq.iterator(array).split('map', function(v){return v * 10; })
     * ```
     */
    Stream scan(const S& begin, std::function<void (S&, const T&)> onScan) {
        auto acc = new S(begin);
        own(acc);
        return createScan([acc]() {
            return convertFrom(*acc);
        },  [onScan, acc](const QVariant & v) {
            onScan(*acc, convert<T>(v));
        });
    }



    template <typename O, typename T, typename INFO, typename = std::enable_if<std::is_same<INFO, Stream::Index>::value>>
    /**
     * @function meta
     * @templateparameter out, optional, if omitted, value type is assumed.
     * @templateparemeter value type
     * @templateparamter requested info
     * @param function that get a value and requested information. F(Info, value)->value
     * or
     * @param function that a requested information. F(Info, value)->value
     * @return Stream
     *
     * Receives information from Stream as an additional input. There are few variants of this Operator, the given function
     * may or may not return value of same type input if get, or no return value at all.
     *
     * Supported Meta information are
     * Stream::Index - index of current input
     * Stream::This - pointer to the current Stream
     *
     */
    Stream meta(std::function<O(const T&, ulong)> f) {
        return createInfo(static_cast<Axq::Stream::InfoValues>(INFO::Index), [f](const QVariant & var, const QVariant & data) {
            return convertFrom<O>(f(convert<T>(var), data.value<ulong>()));
        });
    }

    template <typename O, typename T, typename INFO, typename = std::enable_if<std::is_same<INFO, Stream::This>::value>>
    Stream meta(std::function<O(const T&, Stream)> f) {
        return createInfo(static_cast<Axq::Stream::InfoValues>(INFO::This), [f](const QVariant & var, const QVariant & data) {
            return convertFrom<O>(f(convert<T>(var), data.value<Stream>()));
        });
    }

    template <typename T, typename INFO>
    Stream meta(std::function<T(const T&, ulong)> f) {
        return meta<T, T, INFO>(f);
    }

    template <typename T, typename INFO>
    Stream meta(std::function<T(const T&, Stream)> f) {
        return meta<T, T, INFO>(f);
    }

    template <typename OUT, typename INFO, typename = std::enable_if<std::is_same<INFO, Stream::This>::value>>
    Stream meta(std::function<void (const Stream&)> f) {
        return createInfo(static_cast<Axq::Stream::InfoValues>(INFO::This), [f](const QVariant&, const QVariant & data) {
            return convertFrom<OUT>(f(data.value<Stream>()));
        });
    }

    template <typename O, typename INFO, typename = std::enable_if<std::is_same<INFO, Stream::Index>::value>>
    Stream meta(std::function<O(const ulong&)> f) {
        return createInfo(static_cast<Axq::Stream::InfoValues>(INFO::Index), [f](const QVariant&, const QVariant & data) {
            return convertFrom<O>(f(data.value<ulong>()));
        });
    }


    template <typename INFO>
    Stream meta(std::function<Stream(const Axq::Stream&)> f) {
        return meta<Stream, INFO>(f);
    }

    template <typename INFO>
    Stream meta(std::function<ulong(const ulong&)> f) {
        return meta<ulong, INFO>(f);
    }

    template <typename O, typename I>
    /**
     * @function iterate
     * @templateparam out type
     * @templateparam in type
     * @param onIter function that is called to map iteration value
     * or
     * @param no function
     * @return
     *
     * If the input is iterable (like Axq::Params), each element is individually placed to output.
     * the mapping function is optional.
     *
     */
    Stream iterate(std::function<QList<O> (const I& stream)> onIter) {
        return createList([onIter](const QVariant & var) {
            return convertFrom<QList<O>>(onIter(convert<I>(var)));
        });
    }
    Stream iterate();

    /**
     * @function buffer
     * @param max int (defaults to big value)
     * @return
     *
     * Collects stream input into Axq::Params and output is completed or there size of buffer exceeds given max value.
     *
     */
    Stream buffer(int max = 0xFFFFF - 1) {
        return createBuffer(max);
    }


    /**
     * @function delay
     * @param delayMs milliseconds
     * @return Stream
     *
     * Set a given delay between input and output, please note it is not delay between
     * outputs, for that see request.
     *
     */
    Stream delay(int delayMs) {
        return createDelay(delayMs);
    }


    template < typename T, typename = enable_if_t < !std::is_base_of<QObject, remove_pointer_t<T>>::value >>
    // template<typename T>
    /**
     * @function own
     * @templateparam value type
     * @param value
     * @param optional destructor function
     * @return Stream
     *
     * Takes a given pointer ownership, the destructor is called on value on exit.
     * Optional destructor function.
     *
     * Not implemented for QML
     *
     */
    Stream own(T* value) {
        class Deleter : public Keeper {
        public  :
            Deleter(T* v) : m(v) {}
            std::unique_ptr<T> m;
        };
        return createOwner(new Deleter(value));
    }

    Stream own(QObject* value) {
        applyParent(value);
        return *this;
    }

    template<typename T>
    Stream own(T* value, std::function<void (T*)> d) {
        class Deleter : public Keeper {
        public  :
            Deleter(T* v, std::function<void (T*)> dd) : m(v, dd) {}
            std::unique_ptr<T> m;
        };
        return createOwner(new Deleter(value, d));
    }


    /**
     * @function take
     * @param other Stream
     * @return Stream
     *
     * Take other Stream owned pointers into this Stream
     */
    Stream take(const Stream& other);


    Stream onError(std::function <void (const Param& errorValue, int code)>);

    template<typename T>
    /**
     * @function onError
     * @param error Handler Function, F(T, int)->void
     * @return
     *
     * Called on error
     *
     */
    Stream onError(std::function <void (const T&, int)> f) {
        return onError([f](const QVariant & v, int code) {
            f(convert<T>(v), code);
        });
    }

    /**
     * @function onCompleted
     * @param complete Handler function, F()->void, or F(const Stream&)->void, or F(value)->void
     * @return Stream
     *
     * Called on when Stream is done, no more running, about or already died.
     * if given handler get Stream, that object cannot be streamed any further.
     * if given handler get value, that is the last value before complete.
     *
     */
    Stream onCompleted(std::function <void ()> f);
    Stream onCompleted(std::function <void (const Stream&)> f);

    template<typename T>
    Stream onCompleted(std::function <void (const T&)> last) {
        return createOnCompleted([last](const QVariant & v) {
            last(convert<T>(v));
        });
    }


    /**
     * @function request
     * @param delayMs milliseconds
     *
     * Explicitly request producer output. The optional delay defines a delay between outputs.
     * By default the request(0) is called when a Stream is constructed, but explicit request or
     * defer would override it.
     *
     */
    Stream request(int delayMs = 0);

    /**
     * @function defer
     * @return Stream
     *
     * Stops request from the producer, after defer, request restarts output.
     *
     */
    Stream defer();

    /**
     * @function cancel
     *
     * Imperative call to cancel all producer output, and error handler is called with
     *  StreamCancel as an error value and StreamCancelValue as error code.
     *
     */
    void cancel();


    template < typename T, typename = enable_if_t < !std::is_array<T>::value >>
    /**
     * @function error
     * templateparam error value type, if not given a "string" is expected
     * @param error item
     * @param code
     * @param isFatal
     *
     * Imperative error call.
     *
     */
    void error(const T& err, int code = -1, bool isFatal = false) {
        makeError(convertFrom<T>(err), code, isFatal);
    }

    void error(const char* err, int code = -1, bool isFatal = false) {
        makeError(convertFrom<QString>(err), code, isFatal);
    }


    Stream(StreamBase* ptr, const Stream& parent);
    Stream(ProducerBase* ptr);
    Stream(ProducerBase*, std::nullptr_t);
    Stream();


    /*virtual ~Stream(); using compiler gen stuff*/

protected:
    StreamBase* stream() const;
private:
    static QList<StreamBase*> mapToStream(const QList<Stream>& sources);
    static QList<ProducerBase*> mapToProducer(const QList<Stream>& sources);
    Stream createEach(std::function<void (const QVariant&)>);
    Stream createDelay(int delayMs);
    Stream createBuffer(int max);
    Stream createCompleteFilter(std::function<bool (const QVariant&)>);
    Stream createMap(std::function<QVariant(const QVariant&)>);
    Stream createFilter(std::function<bool (const QVariant&)>);
    Stream createSpawn(std::function<Stream(const QVariant&)>);
    Stream createScan(std::function<QVariant()>, std::function<void (const QVariant&)>);
    Stream createInfo(Axq::Stream::InfoValues intoType, std::function<QVariant(const QVariant&, const QVariant&)>);
    Stream createList(std::function<QVariant(const QVariant&)>);
    Stream createOwner(Keeper* deleter);
    Stream createOnCompleted(std::function<void (const QVariant&)>);
    std::tuple<Stream, Stream> createSplit();
    std::tuple<Stream, Stream> createAsync();
    std::tuple<Stream, Waiter*> createWait();

    void setChild(const Stream& stream);
    void makeError(const QVariant& error, int code, bool isFatal);
    void applyParent(QObject* obj);

// These here to keep private
    static Stream createContainer(const QVariant& container);

    static Stream createRepeater(std::function<QVariant()> function, int intervalMs);
    static Stream createIterator(Keeper* iterator, std::function<bool ()> hasNext, std::function<QVariant()> next);
    static Stream create(std::function<QVariant()> function);
    static Stream create(QIODevice* device, int len);

    template <typename T> static T convert(const QVariant& val);
    template <typename T> static T convert(QVariant& val);

    static constexpr auto EXTRARESERVE = 10;

    template <typename K, typename V> static void convertTo(QHash<K, V>& hash,  const QVariant& val) {
        const auto iterable = val.toHash();
        hash.reserve(iterable.size() + EXTRARESERVE);
        for(auto i = iterable.constBegin(); i != iterable.constEnd(); i++) {
            hash.insert(i.key(), i.value().value<V>());
        }
    }

    template <typename K, typename V> static void convertTo(QMap<K, V>& map,  const QVariant& val) {
        const auto iterable = val.toMap();
        map.reserve(iterable.size() + EXTRARESERVE);
        for(auto i = iterable.constBegin(); i != iterable.constEnd(); i++) {
            map.insert(i.key(), i.value().value<V>());
        }
    }

    template <typename V> static void convertTo(QList<V>& list,  const QVariant& val) {
        const auto iterable = val.toList();
        list.reserve(iterable.size() + EXTRARESERVE);
        for(const auto& i : iterable) {
            list.append(i.value<V>());
        }
    }

    template <typename V> static void convertTo(QVector<V>& list,  const QVariant& val) {
        const auto iterable = val.toList();
        list.reserve(iterable.size() + EXTRARESERVE);
        for(const auto& i : iterable) {
            list.append(i.value<V>());
        }
    }

    template <typename T> static void convertTo(T&,  const QVariant&) {
    }

    template <class ...Arg> static void convertTo(Arg... args) {
        convertTo(args...);
    }


    template <typename T>
    static QVariant convertFrom(const T& val) {
        return QVariant::fromValue<T>(val);
    }

    template <typename T>
    static QVariant convertFrom(const QHash<QString, T>& val) {
        QVariantHash hash;
        for(auto it = val.begin(); it != val.end(); ++it) {
            hash.insert(it.key(), QVariant::fromValue<T>(it.value()));
        }
        return QVariant::fromValue<QVariantHash>(hash);
    }

    template <typename T>
    static QVariant convertFrom(const QMap<QString, T>& val) {
        QVariantMap map;
        for(auto it = val.begin(); it != val.end(); ++it) {
            map.insert(it.key(), QVariant::fromValue<T>(it.value()));
        }
        return QVariant::fromValue<QVariantMap>(map);
    }

    template <typename T>
    static QVariant convertFrom(const QList<T>& val) {
        QVariantList lst;
        lst.reserve(val.size());
        for(const auto& v : val) {
            lst.append(QVariant::fromValue<T>(v));
        }
        return QVariant::fromValue<QVariantList>(lst);
    }


    template <typename T>
    static QVariant convertFrom(const QVector<T>& val) {
        QVariantList lst;
        lst.reserve(val.size());
        for(const auto& v : val) {
            lst.append(QVariant::fromValue<T>(v));
        }
        return QVariant::fromValue<QVariantList>(lst);
    }


private:
    StreamBase* m_ptr = nullptr;
    std::shared_ptr<StreamBase> m_private;
    template <typename T> friend Stream _from(const T& containe);
    template <typename T> friend Stream repeater(int intervalMs, std::function<T()> function);
    template <typename T> friend Stream repeater(int intervalMs, const T& value);
    template <class T, typename> friend T async(const T& stream);
    template <typename ...Args> friend ParamList params(Args...args);
    template <typename T> friend Stream create(std::function<T()> function);
    template <class T, class inputIt> friend Stream iterator(inputIt begin, inputIt end);
    template <typename T, class IT> friend Stream iterator(const IT& it);
    template <typename T> friend Stream iterator(const QString& it);
    friend class Queue;
    AXQSHAREDLIB_EXPORT friend Stream merge(const QList<Stream>& streams);
};
/**
  * @scopeend Stream
  */

/**
 * @class Queue
 *
 * Queue object if given to Stream::create function and then
 * its push function delivers values into Stream.
 *
 */
class AXQSHAREDLIB_EXPORT Queue : public QObject {
    Q_OBJECT
public:
    /**
     * @function Queue
     * @param parent
     */
    Queue(QObject* parent = nullptr);
    template <typename T>
    /**
     * @function push
     * @param value
     *
     * Delivers a given value into Stream.
     *
     */
    void push(const T& value) {
        push(Stream::convertFrom<T>(value));
    }
signals:
    void push(const QVariant& value);
    /**
     * @function complete
     *
     * Stream is completed when its Queue emits complete signal.
     *
     */
    void complete();
};
/**
  * @scopeend Queue
  */

/**
 * @function registerTypes
 * for QML, registerTypes C++ function must ba called before QML initialization.
 */
AXQSHAREDLIB_EXPORT void registerTypes();


template <typename T>
/**
 * @function create
 * @templateparam type of function return value
 * @param function that is called when data is requested on stream
 * @return Stream
 *
 * Generate a stream value from function return value.
 *
 */
Stream create(std::function<T()> function) {
    return Stream::create([function]() {
        return Stream::convertFrom<T>(function());
    });
}

/**
 * @function create
 * @param pointer to Queue object that generates values.If Queue is not having a parent, the Stream will take it.
 * @return Stream
 *
 * Generate a stream value from Queue push
 *
 */
AXQSHAREDLIB_EXPORT Stream create(Queue* queue);


/**
 * @function read
 * @param QIODevice* that is read for data.
 * @param spesifies the length of read data, notably `Stream::OneLine` reads one line and `Stream::Slurp` read all content available.
 * @return Stream
 *
 * Reads data from them the QIODevices on the stream.
 *
 */
AXQSHAREDLIB_EXPORT Stream read(QIODevice* device, int len = Stream::OneLine);

template <typename T>
/**
 * @function repeater
 * @templateparam  type of value
 * @param interval in ms the repeater is called.
 * @param function which generates data on the stream.
 * @return Stream
 *
 * Repeats the data on the stream.
 *
 */
Stream repeater(int intervalMs, std::function<T()> function) {
    return Stream::createRepeater([function]() {
        return Stream::convertFrom<T>(function());
    }, intervalMs);
}


template <typename T>
/**
 * @function repeater
 * @param interval millisecods the repeater is called.
 * @param value that is put on stream
 * @return Stream
 *
 * Repeats the data on the stream.
 *
 */
Stream repeater(int intervalMs, const T& value) {
    return Stream::createRepeater([value]() {return Stream::convertFrom<T>(value);}, intervalMs);
}

template <class T, class inputIt>
/**
 * @function iterator
 * @templateparam type of iterated value
 * @param begin input iterator to start
 * @param end input iterator to end
 * @return Stream
 *
 * Iterates the given input iterator values into stream.
 *
 */
Stream iterator(inputIt begin, inputIt end) {
    class Begin : public Stream::Keeper {
    public:
        Begin(inputIt iptr) : iter(iptr) {}
        inputIt iter;
    };
    auto beginIterator = new Begin(begin);
    return Stream::createIterator(beginIterator, [beginIterator, end]() {
        return beginIterator->iter != end;
    }, [beginIterator, begin]() {
        auto& it = beginIterator->iter;
        const auto var = QVariant::fromValue<T>(*it);
        ++it;
        return var;
    });
}


template <typename T, class IT>
/**
 * @function iterator
 * @templateparam type of iterated value
 * @param java-style iterator
 * @return Stream
 *
 *  Iterates iterator values into stream.
 * "java-style", i.e. given object has hasNext and next functions.
 *
 */
Stream iterator(const IT& it) {
    class Iterator : public Stream::Keeper {
    public:
        Iterator(IT iptr) : iter(iptr) {}
        IT iter;
    };
    auto iter = new Iterator(it);
    return Stream::createIterator(iter, [iter]() {
        return iter->iter.hasNext();
    }, [iter]() {
        return QVariant::fromValue<T>(iter->iter.next());
    });
}

template <class T>
/**
 * @function iterator
 * @param str
 * @return Stream
 *
 * Iterates given string characters into Stream.
 *
 */
Stream iterator(const QString& str) {
    class Iterator : public Stream::Keeper {
    public:
        Iterator(const QString& s) : m_s(s) {}
        bool hasNext() const {return m_it < m_s.length();}
        QChar next() {return m_s[m_it++];}
    private:
        const QString m_s;
        int m_it = 0;
    };
    auto iter = new Iterator(str);
    return Stream::createIterator(iter, [iter]() {
        return iter->hasNext();
    }, [iter]() {
        return QVariant::fromValue<QChar>(iter->next());
    });
}

template <class T>
Stream iterator(const char* cstr) {
    const QString str(cstr);
    return iterator<QString>(str);
}

template <typename T>
Stream _from(const T& container) {
    return Stream::createContainer(Stream::convertFrom(container));
}


/**
 * @function from
 * @param object, object to take, if it has no parent, the parenthood is taken and object is freed upon complete
 * @return Stream
 *
 * Puts given object into the Stream.
 *
 */
AXQSHAREDLIB_EXPORT Stream from(QObject* object);

template <class T>
/**
 * @function from
 * @param container to put into Stream
 * @return Stream
 *
 * If container can resolve to QList, QVector, QHash or QMap its values are iterated into Stream, otherwise
 * value is put into Stream as-is.
 */
Stream from(const T& container, typename std::enable_if < !std::is_pointer<T>::value >::type* = 0) { //no move :(
    return _from(container);
}


AXQSHAREDLIB_EXPORT Stream range(int begin, int end, int step = 1);

template <typename T>
/**
 * @function range
 * @param begin
 * @param end
 * @param step
 * @return Stream
 *
 *
 * Puts values into Stream. Values are assumed to be integers.
 *
 */
Stream range(T begin, T end, T step = 1) {
    return range(static_cast<int>(begin), static_cast<int>(end), static_cast<int>(step));
}


/**
 * @function merge
 * @param List of Streams
 * @return Stream
 *
 * Merge multiple Streams into this Stream.
 *
 */
AXQSHAREDLIB_EXPORT Stream merge(const QList<Stream>& streams);


template <typename T>
T Stream::convert(const QVariant& val) {
    if(val.canConvert<T>()) {
        return val.value<T>();
    }
    T item{};
    convertTo(item, val);
    return item;
}

template <typename T>
T Stream::convert(QVariant& val) {
    if(val.canConvert<T>()) {
        return val.value<T>();
    }
    T item{};
    convertTo(item, val);
    return item;
}

template <typename ...Args>
/**
 * @function params
 * @param args
 * @return ParamList
 *
 * Utility to collect values into ParamList
 *
 * Example
 * ```
        Axq::iterator<bool>(bitset->begin(), bitset->end())
                .meta<Axq::ParamList, bool, Axq::Stream::Index>([](bool b, ulong index){
            return Axq::params(b, index);
        })
                .filter<Axq::ParamList>([len](const Axq::ParamList& param){
                ...
 * ```
 */
ParamList params(Args...args) {
    ParamList list;
    list.reserve(sizeof...(Args));
    _toList(list, Stream::convertFrom<Args>(args)...);
    return list;
}


} // namespace

/**
  * @scopeend Axq
  */

Q_DECLARE_METATYPE(Axq::Stream)
Q_DECLARE_METATYPE(std::function<Axq::Stream()>)


#endif // AXQ_H
