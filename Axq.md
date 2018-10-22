Axq
=====
Functional Reactive Qt framework
--------------------------------

Axq provides Functional Reactive programming approach with Qt. Axq implements set of Providers,
classes that generate data into Stream. Stream is a way to handle and manipulate data using
Operators. Stream ends to error or complete handler.

```
Producer->Operator->Operator->Operator->Complete_Handler.  
```
There are different kind of Producers, e.g. Range producers increasing integers in given range of values. When
end of range is reached the given complete handler function is called. Operators are given functions that does
a certain operator for data received from stream. For example Map operator transforms from received data to another
value into Stream.

QML example to generate ASCII values:
```
    function showAscii() {  
       lines.text = ""  
       Axq.range(0, 255, 1).filter(function(x){  
                        return x > 0x20 && x < 0x7E  
                    })  
        .map(function(x){  
                 return  x.toString() + ' ' + String.fromCharCode(x) + (x % 5 ? "\t" : '')  
            })  
        .each(function(acc){  
                 lines.text += acc  
             })  
    }  
```
Here, in my test code and examples I mostly use chained notation as that apt well with idea of Streams, however
nothing prevents code written in more imperative manner.

```
    Axq::Range(0, 255, 1).filter(...  
```
vs.
```
auto producer = Axq::Range(0, 255, 1);  
auto filtered = producer.filter(...  
```

The are quite equal APIs for both C++ and Javascript for QML. Streams are mostly asynchronous within created thread,
however C++ API provides operators use multiple threads with Stream.

Besides just operators there are a good collection of functions to manage the Stream. C++ memory managed is &quot;automated&quot;,
so that Stream can just be created and then it will deleted some time after completion. For C++ there are operators `own`
and `take` to take dynamically created items ownership into Stream and thus released when Stream is released.

* [Axq ](#axq)
* [ParamList ](#paramlist)
* [Param ](#param)
* [void tie(const ParamList& list, Args& ...args) ](#void-tie-const-paramlist-list-args-args-)
* [T get(const ParamList& list) ](#t-get-const-paramlist-list-)
* [Stream ](#stream)
* [Stream each(std::function<void (const T&)> onEach) ](#stream-each-std-functionvoid-const-t-oneach-)
* [Stream map(std::function< O(const I&) > onMap) ](#stream-map-std-function-o-const-i-onmap-)
* [Stream async() ](#stream-async-)
* [Stream async(Stream(Stream::*f)(Params...), Args&& ... args) ](#stream-async-stream-stream-f-params-args-args-)
* [Stream split(Stream(Stream::*f)(Params...), Args&& ... args) ](#stream-split-stream-stream-f-params-args-args-)
* [Stream filter(std::function<bool (const T&)> onFilter) ](#stream-filter-std-functionbool-const-t-onfilter-)
* [Stream waitComplete(std::function<Stream(const T&)> onWait) ](#stream-waitcomplete-std-functionstream-const-t-onwait-)
* [Stream completeEach(std::function<bool (const T&)> onPass) ](#stream-completeeach-std-functionbool-const-t-onpass-)
* [Stream completeEach() ](#stream-completeeach-)
* [void complete() ](#void-complete-)
* [Stream wait(O* sender,  F signal) ](#stream-wait-o-sender-f-signal-)
* [Stream scan(const S& begin, std::function<void (S&, const T&)> onScan) ](#stream-scan-const-s-begin-std-functionvoid-s-const-t-onscan-)
* [Stream meta(std::function<O(const T&, ulong)> f) ](#stream-meta-std-functiono-const-t-ulong-f-)
* [Stream iterate(std::function<QList<O> (const I& stream)> onIter) ](#stream-iterate-std-functionqlisto-const-i-stream-oniter-)
* [Stream buffer(int max = 0xFFFFF - 1) ](#stream-buffer-int-max-0xfffff-1-)
* [Stream delay(int delayMs) ](#stream-delay-int-delayms-)
* [Stream own(T* value) ](#stream-own-t-value-)
* [Stream take(const Stream& other) ](#stream-take-const-stream-other-)
* [Stream onError(std::function <void (const T&, int)> f) ](#stream-onerror-std-function-void-const-t-int-f-)
* [Stream onCompleted(std::function <void ()> f) ](#stream-oncompleted-std-function-void-f-)
* [Stream request(int delayMs = 0) ](#stream-request-int-delayms-0-)
* [Stream defer() ](#stream-defer-)
* [void cancel() ](#void-cancel-)
* [void error(const T& err, int code = -1, bool isFatal = false) ](#void-error-const-t-err-int-code-1-bool-isfatal-false-)
* [Queue ](#queue)
* [Queue(QObject* parent = nullptr) ](#queue-qobject-parent-nullptr-)
* [void push(const T& value) ](#void-push-const-t-value-)
* [void complete() ](#void-complete-)
* [ void registerTypes() ](#-void-registertypes-)
* [Stream create(std::function<T()> function) ](#stream-create-std-functiont-function-)
* [ Stream create(Queue* queue) ](#-stream-create-queue-queue-)
* [ Stream read(QIODevice* device, int len = Stream::OneLine) ](#-stream-read-qiodevice-device-int-len-stream-oneline-)
* [Stream repeater(int intervalMs, std::function<T()> function) ](#stream-repeater-int-intervalms-std-functiont-function-)
* [Stream repeater(int intervalMs, const T& value) ](#stream-repeater-int-intervalms-const-t-value-)
* [Stream iterator(inputIt begin, inputIt end) ](#stream-iterator-inputit-begin-inputit-end-)
* [Stream iterator(const IT& it) ](#stream-iterator-const-it-it-)
* [Stream iterator(const QString& str) ](#stream-iterator-const-qstring-str-)
* [ Stream from(QObject* object) ](#-stream-from-qobject-object-)
* [Stream from(const T& container, typename std::enable_if < !std::is_pointer<T>::value >::type* = 0) ](#stream-from-const-t-container-typename-std-enable_if-std-is_pointert-value-type-0-)
* [Stream range(T begin, T end, T step = 1) ](#stream-range-t-begin-t-end-t-step-1-)
* [ Stream merge(const QList<Stream>& streams) ](#-stream-merge-const-qliststream-streams-)
* [ParamList params(Args...args) ](#paramlist-params-args-args-)


###### *Axq is published under GNU LESSER GENERAL PUBLIC LICENSE, Version 3*
###### *Copyright Markus Mertama 2018*, generated at Mon Oct 22 2018 
_____
---
### Axq 
Axq is using *Axq* namespace.

____________________________
---
#### ParamList 
###### When there is need for multiple input-output parameters a Axq::Params utility will help. 
Generic parameter container to use multiple parameter within monadic functions.

##### void tie(const ParamList& list, Args& ...args) 
Apply list values to arguments
###### *Param:* ParamList object to read from 
###### *Param:* list of out parameters to read values to 


Example:
```
stream.each<Axq::ParamList>([this](const Axq::ParamList& pair){  
QString key; int count;  
Axq::tie(pair, key, count);  
print("\"", key, "\" exists ", count, " times");  
```

##### T get(const ParamList& list) 
Get a nth parameter from a list
###### *Template arg:* index of parameter 
###### *Template arg:* type of parameter 
###### *Param:* ParamList object to read from 
###### *Return:* value 

#####  void registerTypes() 
for QML, registerTypes C++ function must ba called before QML initialization.
##### Stream create(std::function<T()> function) 
###### *Template arg:* type of function return value 
###### *Param:* function that is called when data is requested on stream 
###### *Return:* Stream 

Generate a stream value from function return value.

#####  Stream create(Queue* queue) 
###### *Param:* pointer to Queue object that generates values.If Queue is not having a parent, the Stream will take it. 
###### *Return:* Stream 

Generate a stream value from Queue push

#####  Stream read(QIODevice* device, int len = Stream::OneLine) 
###### *Param:* QIODevice* that is read for data. 
###### *Param:* spesifies the length of read data, notably `Stream::OneLine` reads one line and `Stream::Slurp` read all content available. 
###### *Return:* Stream 

Reads data from them the QIODevices on the stream.

##### Stream repeater(int intervalMs, std::function<T()> function) 
###### *Template arg:* type of value 
###### *Param:* interval in ms the repeater is called. 
###### *Param:* function which generates data on the stream. 
###### *Return:* Stream 

Repeats the data on the stream.

##### Stream repeater(int intervalMs, const T& value) 
###### *Param:* interval millisecods the repeater is called. 
###### *Param:* value that is put on stream 
###### *Return:* Stream 

Repeats the data on the stream.

##### Stream iterator(inputIt begin, inputIt end) 
###### *Template arg:* type of iterated value 
###### *Param:* begin input iterator to start 
###### *Param:* end input iterator to end 
###### *Return:* Stream 

Iterates the given input iterator values into stream.

##### Stream iterator(const IT& it) 
###### *Template arg:* type of iterated value 
###### *Param:* java-style iterator 
###### *Return:* Stream 

Iterates iterator values into stream.
&quot;java-style&quot;, i.e. given object has hasNext and next functions.

##### Stream iterator(const QString& str) 
###### *Param:* str 
###### *Return:* Stream 

Iterates given string characters into Stream.

#####  Stream from(QObject* object) 
###### *Param:* object, object to take, if it has no parent, the parenthood is taken and object is freed upon complete 
###### *Return:* Stream 

Puts given object into the Stream.

##### Stream from(const T& container, typename std::enable_if < !std::is_pointer<T>::value >::type* = 0) 
###### *Param:* container to put into Stream 
###### *Return:* Stream 

If container can resolve to QList, QVector, QHash or QMap its values are iterated into Stream, otherwise
value is put into Stream as-is.
##### Stream range(T begin, T end, T step = 1) 
###### *Param:* begin 
###### *Param:* end 
###### *Param:* step 
###### *Return:* Stream 


Puts values into Stream. Values are assumed to be integers.

#####  Stream merge(const QList<Stream>& streams) 
###### *Param:* List of Streams 
###### *Return:* Stream 

Merge multiple Streams into this Stream.

##### ParamList params(Args...args) 
###### *Param:* args 
###### *Return:* ParamList 

Utility to collect values into ParamList

Example
```
        Axq::iterator<bool>(bitset->begin(), bitset->end())  
                .meta<Axq::ParamList, bool, Axq::Stream::Index>([](bool b, ulong index){  
            return Axq::params(b, index);  
        })  
                .filter<Axq::ParamList>([len](const Axq::ParamList& param){  
                ...  
```
---
---
#### Param 
Generic parameter to use with ParamList container

---
---
#### Stream 

Handle to Stream. The Stream is created by producer functions, e.g. `from` and `create`
and then operators like `each` and `map` are applied on it to manage and control the data.
The producer functions and operators returns a Stream object so calls can be chained as
done in the test applications and examples, but naturally there is no reason just call
them individually.

For example two examples below are quite equal.
```
    auto rep = Axq::range(1, 10).each<int>([](int v){...}).map<int, int>([](int v){...});  
```
and
```
    auto rep = Axq::range(1, 10);  
    rep.each<int>([](int v){...});  
    rep.map<int, int>([](int v){...});  
```

##### Stream each(std::function<void (const T&)> onEach) 
###### *Template arg:* stream type 
###### *Param:* onEach function, F(value)->void 
###### *Return:* Stream 

Each does nothing to Stream, but a given function is called for each output.

##### Stream map(std::function< O(const I&) > onMap) 
###### *Template arg:* mapped stream type out 
###### *Template arg:* stream type in 
###### *Param:* onMap function, F(value)->value 
###### *Return:* Stream 

Applies Stream a function that defines the mapping from input to output.

##### Stream async() 
###### *Return:* Stream 

Moves this Stream in its own thread.

There is no QML implementation of this function.

##### Stream async(Stream(Stream::*f)(Params...), Args&& ... args) 
###### *Param:* pointer to operator 
###### *Param:* argument list of operator parameters 
###### *Return:* Stream 
s
Excetes a given Stream Operator in its own thread.

Example:
```
async(&Axq::Stream::each<QString>, [](const QString& out){  
...this is excuted in own thread...  
});  
```

There is no QML implementation of this function

##### Stream split(Stream(Stream::*f)(Params...), Args&& ... args) 
###### *Param:* pointer to operator 
###### *Param:* argument list of operator parameters 
###### *Return:* Stream 

Split current thread input to original and one from given Operator with given parameters applied.
The output value is a Axq::Parameters where the first parameter is the original and second output from the
Operator's.

For QML the name of the function is given as a string.

##### Stream filter(std::function<bool (const T&)> onFilter) 
###### *Template arg:* type 
###### *Param:* onFilter function, F(value)->boolean 
###### *Return:* Stream 

Applies stream function that removes input from output if returns false

##### Stream waitComplete(std::function<Stream(const T&)> onWait) 
###### *Template arg:* type 
###### *Param:* onWait function, F(value)->Stream 
###### *Return:* Stream 

The parameter function is expected a return a nested stream. The outter
Stream will not be completed until a given stream is completed. Please note that Axq
is asynchronous and therefore the outter producer is not stopped or deferred.

##### Stream completeEach(std::function<bool (const T&)> onPass) 
###### *Template arg:* type 
###### *Param:* onPass function, F(value)->boolean 
###### *Return:* Stream 

Does complete the Stream when a given function returns true.

##### Stream completeEach() 
###### *Return:*  

Complete the Stream unconditionally

##### void complete() 
Imperative complete the Stream.

##### Stream wait(O* sender,  F signal) 
###### *Param:* sender object 
###### *Param:* Qt signal 
###### *Return:* Sender 

Prevents the Stream not to be completed until the given signal is emitted.
##### Stream scan(const S& begin, std::function<void (S&, const T&)> onScan) 
###### *Template arg:* stream type 
###### *Param:* begin initial value 
###### *Param:* onScan function, the first parameter is the accumulated value, F(reference to accumulator, value)->void 
###### *Return:* Stream 

The input is collected into accumulator and then output when parent producer is completed.

C++ Example:
```
stream.scan<int>(0, [](int acc, int value){ acc += value;});  
```

QML Example:
```
Axq.iterator(array).split('map', function(v){return v * 10; })  
```
##### Stream meta(std::function<O(const T&, ulong)> f) 
##### out, optional, if omitted, value type is assumed. 
##### value type 
##### requested info 
###### *Param:* function that get a value and requested information. F(Info, value)->value 
or
###### *Param:* function that a requested information. F(Info, value)->value 
###### *Return:* Stream 

Receives information from Stream as an additional input. There are few variants of this Operator, the given function
may or may not return value of same type input if get, or no return value at all.

Supported Meta information are
Stream::Index - index of current input
Stream::This - pointer to the current Stream

##### Stream iterate(std::function<QList<O> (const I& stream)> onIter) 
###### *Template arg:* out type 
###### *Template arg:* in type 
###### *Param:* onIter function that is called to map iteration value 
or
###### *Param:* no function 
###### *Return:*  

If the input is iterable (like Axq::Params), each element is individually placed to output.
the mapping function is optional.

##### Stream buffer(int max = 0xFFFFF - 1) 
###### *Param:* max int (defaults to big value) 
###### *Return:*  

Collects stream input into Axq::Params and output is completed or there size of buffer exceeds given max value.

##### Stream delay(int delayMs) 
###### *Param:* delayMs milliseconds 
###### *Return:* Stream 

Set a given delay between input and output, please note it is not delay between
outputs, for that see request.

##### Stream own(T* value) 
###### *Template arg:* value type 
###### *Param:* value 
###### *Param:* optional destructor function 
###### *Return:* Stream 

Takes a given pointer ownership, the destructor is called on value on exit.
Optional destructor function.

Not implemented for QML

##### Stream take(const Stream& other) 
###### *Param:* other Stream 
###### *Return:* Stream 

Take other Stream owned pointers into this Stream
##### Stream onError(std::function <void (const T&, int)> f) 
###### *Param:* error Handler Function, F(T, int)->void 
###### *Return:*  

Called on error

##### Stream onCompleted(std::function <void ()> f) 
###### *Param:* complete Handler function, F()->void, or F(const Stream&)->void, or F(value)->void 
###### *Return:* Stream 

Called on when Stream is done, no more running, about or already died.
if given handler get Stream, that object cannot be streamed any further.
if given handler get value, that is the last value before complete.

##### Stream request(int delayMs = 0) 
###### *Param:* delayMs milliseconds 

Explicitly request producer output. The optional delay defines a delay between outputs.
By default the request(0) is called when a Stream is constructed, but explicit request or
defer would override it.

##### Stream defer() 
###### *Return:* Stream 

Stops request from the producer, after defer, request restarts output.

##### void cancel() 

Imperative call to cancel all producer output, and error handler is called with
StreamCancel as an error value and StreamCancelValue as error code.

##### void error(const T& err, int code = -1, bool isFatal = false) 
templateparam error value type, if not given a &quot;string&quot; is expected
###### *Param:* error item 
###### *Param:* code 
###### *Param:* isFatal 

Imperative error call.

---
---
#### Queue 

Queue object if given to Stream::create function and then
its push function delivers values into Stream.

##### Queue(QObject* parent = nullptr) 
###### *Param:* parent 
##### void push(const T& value) 
###### *Param:* value 

Delivers a given value into Stream.

##### void complete() 

Stream is completed when its Queue emits complete signal.

---
###### Generated by MarkupMaker, (c) Markus Mertama 2018 
