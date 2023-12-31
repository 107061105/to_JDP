// 2019/04/09 - modified by Tsung-Wei Huang
//  - removed silent_dispatch method
//
// 2019/03/12 - modified by Chun-Xun Lin
//  - added framework
//
// 2019/02/11 - modified by Tsung-Wei Huang
//  - refactored run_until
//  - added allocator to topologies
//  - changed to list for topologies
//
// 2019/02/10 - modified by Chun-Xun Lin
//  - added run_n to execute framework
//  - finished first peer-review with TW
//
// 2018/07 - 2019/02/09 - missing logs
//
// 2018/06/30 - created by Tsung-Wei Huang
//  - added BasicTaskflow template

// TODO items:
// 1. come up with a better way to remove the "joined" links 
//    during the execution of a static node (1st layer)
//

#pragma once

#include "topology.hpp"

namespace tf {

/** @class BasicTaskflow

@brief The base class to derive a taskflow class.

@tparam E: executor type to use in this taskflow

This class is the base class to derive a taskflow class. 
It inherits all public methods to create tasks from tf::FlowBuilder
and defines means to execute task dependency graphs.

*/
template <template <typename...> typename E>
class BasicTaskflow : public FlowBuilder {

  using StaticWork  = typename Node::StaticWork;
  using DynamicWork = typename Node::DynamicWork;
  
  struct Closure {

    friend class BasicTaskflow;
  
    Closure() = default;
    Closure(const Closure&) = default;
    Closure(BasicTaskflow&, Node&);

    Closure& operator = (const Closure&) = default;
    
    void operator ()() const;

    BasicTaskflow* taskflow {nullptr};
    Node*          node     {nullptr};
  };
  
  public:
  
  /**
  @typedef Executor

  @brief alias of executor type
  */
  using Executor = E<Closure>;
    
    /**
    @brief constructs the taskflow with std::thread::hardware_concurrency worker threads
    */
    explicit BasicTaskflow();
    
    /**
    @brief constructs the taskflow with N worker threads
    */
    explicit BasicTaskflow(unsigned N);
    
    /**
    @brief constructs the taskflow with a given executor
    */
    explicit BasicTaskflow(std::shared_ptr<Executor> executor);
    
    /**
    @brief destructs the taskflow

    Destructing a taskflow object will first wait for all running topologies to finish
    and then clean up all associated data storages.
    */
    ~BasicTaskflow();
    
    /**
    @brief shares ownership of the executor associated with this taskflow object

    @return a std::shared_ptr of the executor
    */
    std::shared_ptr<Executor> share_executor();
    
    /**
    @brief dispatches the present graph to threads and returns immediately

    @return a std::shared_future to access the execution status of the dispatched graph
    */
    std::shared_future<void> dispatch();
    
    /**
    @brief dispatches the present graph to threads and run a callback when the graph completes

    @return a std::shared_future to access the execution status of the dispatched graph
    */
    template <typename C>
    std::shared_future<void> dispatch(C&&);
  
    /**
    @brief dispatches the present graph to threads and wait for all topologies to complete
    */
    void wait_for_all();

    /**
    @brief blocks until all running topologies complete and then
           cleans up all associated storages
    */
    void wait_for_topologies();
    
    /**
    @brief dumps the present task dependency graph to a std::ostream in DOT format

    @param ostream a std::ostream target
    */
    void dump(std::ostream& ostream) const;

    /**
    @brief dumps the present topologies to a std::ostream in DOT format

    @param ostream a std::ostream target
    */
    void dump_topologies(std::ostream& ostream) const;
    
    /**
    @brief queries the number of nodes in the present task dependency graph
    */
    size_t num_nodes() const;

    /**
    @brief queries the number of worker threads in the associated executor
    */
    size_t num_workers() const;

    /**
    @brief queries the number of existing topologies
    */
    size_t num_topologies() const;
    
    /**
    @brief dumps the present task dependency graph in DOT format to a std::string
    */
    std::string dump() const;
    
    /**
    @brief dumps the existing topologies in DOT format to a std::string
    */
    std::string dump_topologies() const;

    /**
    @brief runs the framework once
    
    @param framework a tf::Framework object

    @return a std::shared_future to access the execution status of the framework
    */
    std::shared_future<void> run(Framework& framework);

    /**
    @brief runs the framework once and invoke a callback upon completion

    @param framework a tf::Framework object 
    @param callable a callable object to be invoked after this run

    @return a std::shared_future to access the execution status of the framework
    */
    template<typename C>
    std::shared_future<void> run(Framework& framework, C&& callable);

    /**
    @brief runs the framework for N times
    
    @param framework a tf::Framework object
    @param N number of runs

    @return a std::shared_future to access the execution status of the framework
    */
    std::shared_future<void> run_n(Framework& framework, size_t N);

    /**
    @brief runs the framework for N times and invokes a callback upon completion

    @param framework a tf::Framework 
    @param N number of runs
    @param callable a callable object to be invoked after this run

    @return a std::shared_future to access the execution status of the framework
    */
    template<typename C>
    std::shared_future<void> run_n(Framework& framework, size_t N, C&& callable);

    /**
    @brief runs the framework multiple times until the predicate becomes true and invoke a callback

    @param framework a tf::Framework 
    @param predicate a boolean predicate to return true for stop

    @return a std::shared_future to access the execution status of the framework
    */
    template<typename P>
    std::shared_future<void> run_until(Framework& framework, P&& predicate);

    /**
    @brief runs the framework multiple times until the predicate becomes true and invoke a callback

    @param framework a tf::Framework 
    @param predicate a boolean predicate to return true for stop
    @param callable a callable object to be invoked after this run

    @return a std::shared_future to access the execution status of the framework
    */
    template<typename P, typename C>
    std::shared_future<void> run_until(Framework& framework, P&& predicate, C&& callable);

  private:
    
    Graph _graph;

    std::shared_ptr<Executor> _executor;

    //std::list<Topology, SingularAllocator<Topology>> _topologies;
    std::list<Topology> _topologies;

    void _schedule(Node&);
    void _schedule(PassiveVector<Node*>&);
    void _set_module_node(Node&);
};

// ============================================================================
// BasicTaskflow::Closure Method Definitions
// ============================================================================

// Function: run
template <template <typename...> typename E>
std::shared_future<void> BasicTaskflow<E>::run(Framework& f) {
  return run_n(f, 1, [](){});
}

// Function: run
template <template <typename...> typename E>
template <typename C>
std::shared_future<void> BasicTaskflow<E>::run(Framework& f, C&& c) {
  static_assert(std::is_invocable<C>::value);
  return run_n(f, 1, std::forward<C>(c));
}

// Function: run_n
template <template <typename...> typename E>
std::shared_future<void> BasicTaskflow<E>::run_n(Framework& f, size_t repeat) {
  return run_n(f, repeat, [](){});
}

// Function: run_n
template <template <typename...> typename E>
template <typename C>
std::shared_future<void> BasicTaskflow<E>::run_n(Framework& f, size_t repeat, C&& c) {
  return run_until(f, [repeat]() mutable { return repeat-- == 0; }, std::forward<C>(c));
}

// Function: run_until
template <template <typename...> typename E>
template <typename P>
std::shared_future<void> BasicTaskflow<E>::run_until(Framework& f, P&& predicate) {
  return run_until(f, std::forward<P>(predicate), [](){});
}

// Function: run_until
template <template <typename...> typename E>
template <typename P, typename C>
std::shared_future<void> BasicTaskflow<E>::run_until(Framework& f, P&& predicate, C&& c) {

  // Predicate must return a boolean value
  static_assert(std::is_invocable_v<C> && std::is_invocable_v<P>);

  if(std::invoke(predicate)) {
    return std::async(std::launch::deferred, [](){}).share();
  }
  
  // create a topology for this run
  auto &tpg = _topologies.emplace_back(f, std::forward<P>(predicate));

  // Iterative execution to avoid stack overflow
  if(num_workers() == 0) {

    // Clear last execution data & Build precedence between nodes and target
    tpg._bind(f._graph);

    do {
      _schedule(tpg._sources);
      tpg._recover_num_sinks();
    } while(!std::invoke(tpg._predicate));

    std::invoke(c);
    tpg._promise.set_value();

    return tpg._future;
  }

  // Multi-threaded execution.
  std::scoped_lock lock(f._mtx);

  f._topologies.push_back(&tpg);

  bool run_now = (f._topologies.size() == 1);

  if(run_now) {
    tpg._bind(f._graph);
  }

  tpg._work = [&f, c=std::forward<C>(c), this] () mutable {
      
    // case 1: we still need to run the topology again
    if(!std::invoke(f._topologies.front()->_predicate)) {
      f._topologies.front()->_recover_num_sinks();
      _schedule(f._topologies.front()->_sources); 
    }
    // case 2: the final run of this topology
    else {
      std::invoke(c);

      f._mtx.lock();

      // If there is another run (interleave between lock)
      if(f._topologies.size() > 1) {

        // Set the promise
        f._topologies.front()->_promise.set_value();
        f._topologies.pop_front();
        f._topologies.front()->_bind(f._graph);
        f._mtx.unlock();
        _schedule(f._topologies.front()->_sources);
      }
      else {
        assert(f._topologies.size() == 1);
        // Need to back up the promise first here becuz framework might be 
        // destroy before taskflow leaves
        auto &p = f._topologies.front()->_promise; 
        f._topologies.pop_front();
        f._mtx.unlock();
       
        // We set the promise in the end in case framework leaves before taskflow
        p.set_value();
      }
    }
  };

  if(run_now) {
    _schedule(tpg._sources);
  }

  return tpg._future;
}

// Constructor
template <template <typename...> typename E>
BasicTaskflow<E>::Closure::Closure(BasicTaskflow& t, Node& n) : 
  taskflow{&t}, node {&n} {
}

// Operator ()
template <template <typename...> typename E>
void BasicTaskflow<E>::Closure::operator () () const {

  // Here we need to fetch the num_successors first to avoid the invalid memory
  // access caused by topology clear.
  const auto num_successors = node->num_successors();
  
  // regular node type
  // The default node work type. We only need to execute the callback if any.
  if(auto index=node->_work.index(); index == 0) {
    if(node->_module != nullptr) {
      bool first_time = !node->is_spawned();
      std::invoke(std::get<StaticWork>(node->_work));
      if(first_time) {
        return ;
      }
    }
    else {
      if(auto &f = std::get<StaticWork>(node->_work); f != nullptr){
        std::invoke(f);
      }
    }
  }
  // subflow node type 
  else {
    
    // Clear the subgraph before the task execution
    if(!node->is_spawned()) {
      node->_subgraph.emplace();
    }
   
    SubflowBuilder fb(*(node->_subgraph));

    std::invoke(std::get<DynamicWork>(node->_work), fb);
    
    // Need to create a subflow if first time & subgraph is not empty 
    if(!node->is_spawned()) {
      node->set_spawned();
      if(!node->_subgraph->empty()) {
        // For storing the source nodes
        PassiveVector<Node*> src; 
        for(auto& n : *(node->_subgraph)) {
          n._topology = node->_topology;
          n.set_subtask();
          if(n.num_successors() == 0) {
            if(fb.detached()) {
              node->_topology->_num_sinks ++;
            }
            else {
              n.precede(*node);
            }
          }
          if(n.num_dependents() == 0) {
            src.push_back(&n);
          }
        }

        taskflow->_schedule(src);

        if(!fb.detached()) {
          return;
        }
      }
    }
  } // End of DynamicWork -----------------------------------------------------
  
  // Recover the runtime change due to dynamic tasking except the target & spawn tasks 
  // This must be done before scheduling the successors, otherwise this might cause 
  // race condition on the _dependents
  //if(num_successors && !node->_subtask) {
  if(!node->is_subtask()) {
    // Only dynamic tasking needs to restore _dependents
    // TODO:
    if(node->_work.index() == 1 &&  !node->_subgraph->empty()) {
      while(!node->_dependents.empty() && node->_dependents.back()->is_subtask()) {
        node->_dependents.pop_back();
      }
    }
    node->_num_dependents = static_cast<int>(node->_dependents.size());
    node->unset_spawned();
  }

  // At this point, the node storage might be destructed.
  for(size_t i=0; i<num_successors; ++i) {
    if(--(node->_successors[i]->_num_dependents) == 0) {
      taskflow->_schedule(*(node->_successors[i]));
    }
  }

  // A node without any successor should check the termination of topology
  if(num_successors == 0) {
    if(--(node->_topology->_num_sinks) == 0) {

      // This is the last executing node 
      bool is_framework = node->_topology->_handle.index() == 1;
      if(node->_topology->_work != nullptr) {
        std::invoke(node->_topology->_work);
      }
      if(!is_framework) {
        node->_topology->_promise.set_value();
      }
    }
  }
}

// ============================================================================
// BasicTaskflow Method Definitions
// ============================================================================

// Constructor
template <template <typename...> typename E>
BasicTaskflow<E>::BasicTaskflow() : 
  FlowBuilder {_graph},
  _executor {std::make_shared<Executor>(std::thread::hardware_concurrency())} {
}

// Constructor
template <template <typename...> typename E>
BasicTaskflow<E>::BasicTaskflow(unsigned N) : 
  FlowBuilder {_graph},
  _executor {std::make_shared<Executor>(N)} {
}

// Constructor
template <template <typename...> typename E>
BasicTaskflow<E>::BasicTaskflow(std::shared_ptr<Executor> e) :
  FlowBuilder {_graph},
  _executor {std::move(e)} {

  if(_executor == nullptr) {
    TF_THROW(Error::EXECUTOR, 
      "failed to construct taskflow (executor cannot be null)"
    );
  }
}

// Destructor
template <template <typename...> typename E>
BasicTaskflow<E>::~BasicTaskflow() {
  wait_for_topologies();
}

// Function: num_nodes
template <template <typename...> typename E>
size_t BasicTaskflow<E>::num_nodes() const {
  return _graph.size();
}

// Function: num_workers
template <template <typename...> typename E>
size_t BasicTaskflow<E>::num_workers() const {
  return _executor->num_workers();
}

// Function: num_topologies
template <template <typename...> typename E>
size_t BasicTaskflow<E>::num_topologies() const {
  return _topologies.size();
}

// Function: share_executor
template <template <typename...> typename E>
std::shared_ptr<typename BasicTaskflow<E>::Executor> BasicTaskflow<E>::share_executor() {
  return _executor;
}

// Procedure: dispatch 
template <template <typename...> typename E>
std::shared_future<void> BasicTaskflow<E>::dispatch() {

  if(_graph.empty()) {
    return std::async(std::launch::deferred, [](){}).share();
  }

  auto& topology = _topologies.emplace_back(std::move(_graph));
 
  _schedule(topology._sources);

  return topology._future;
}


// Procedure: dispatch with registered callback
template <template <typename...> typename E>
template <typename C>
std::shared_future<void> BasicTaskflow<E>::dispatch(C&& c) {

  if(_graph.empty()) {
    c();
    return std::async(std::launch::deferred, [](){}).share();
  }

  auto& topology = _topologies.emplace_back(std::move(_graph), std::forward<C>(c));

  _schedule(topology._sources);

  return topology._future;
}

// Procedure: wait_for_all
template <template <typename...> typename E>
void BasicTaskflow<E>::wait_for_all() {
  if(!_graph.empty()) {
    dispatch();
  }
  wait_for_topologies();
}

// Procedure: wait_for_topologies
template <template <typename...> typename E>
void BasicTaskflow<E>::wait_for_topologies() {
  for(auto& t: _topologies){
    t._future.get();
  }
  _topologies.clear();
}

// Procedure: _schedule
// The main procedure to schedule a give task node.
// Each task node has two types of tasks - regular and subflow.
template <template <typename...> typename E>
void BasicTaskflow<E>::_schedule(Node& node) {
  if(node._module != nullptr && !node.is_spawned()) {
    _set_module_node(node);
  }
  _executor->emplace(*this, node);
}


// Procedure: _schedule
// The main procedure to schedule a set of task nodes.
// Each task node has two types of tasks - regular and subflow.
template <template <typename...> typename E>
void BasicTaskflow<E>::_schedule(PassiveVector<Node*>& nodes) {
  std::vector<Closure> closures;
  closures.reserve(nodes.size());
  for(auto src : nodes) {
    if(src->_module != nullptr && !src->is_spawned()) {
      _set_module_node(*src);
    }
    closures.emplace_back(*this, *src);
  }
  _executor->batch(closures);
}


template <template <typename...> typename E>
void BasicTaskflow<E>::_set_module_node(Node& node) {

  node._work = [&node, this, tgt{PassiveVector<Node*>()}] () mutable {

    // second time to enter this context
    if(node.is_spawned()) {
      node._dependents.resize(node._dependents.size()-tgt.size());
      for(auto& t: tgt) {
        t->_successors.clear();
      }
      return ;
    }

    // first time to enter this context
    node.set_spawned();

    PassiveVector<Node*> src;

    for(auto &n: node._module->_graph) {
      n._topology = node._topology;
      if(n.num_dependents() == 0) {
        src.push_back(&n);
      }
      if(n.num_successors() == 0) {
        n.precede(node);
        tgt.push_back(&n);
      }
    }

    _schedule(src);
  };
}


// Function: dump_topologies
template <template <typename...> typename E>
std::string BasicTaskflow<E>::dump_topologies() const {
  
  std::ostringstream os;

  for(const auto& tpg : _topologies) {
    tpg.dump(os);
  }
  
  return os.str();
}

// Function: dump_topologies
template <template <typename...> typename E>
void BasicTaskflow<E>::dump_topologies(std::ostream& os) const {
  for(const auto& tpg : _topologies) {
    tpg.dump(os);
  }
}

// Function: dump
template <template <typename...> typename E>
void BasicTaskflow<E>::dump(std::ostream& os) const {

  os << "digraph Taskflow {\n";
  
  for(const auto& node : _graph) {
    node.dump(os);
  }

  os << "}\n";
}

// Function: dump
// Dumps the taskflow in graphviz. 
// The result can be viewed at http://www.webgraphviz.com/.
template <template <typename...> typename E>
std::string BasicTaskflow<E>::dump() const {
  std::ostringstream os;
  dump(os); 
  return os.str();
}


}  // end of namespace tf ----------------------------------------------------

namespace tf {

class Executor {
  
  struct Worker {
    std::minstd_rand rdgen { std::random_device{}() };
    WorkStealingQueue<Node*> queue;
  };
    
  struct PerThread {
    Executor* pool {nullptr}; 
    int worker_id  {-1};
  };

  public:
    
    /**
    @brief constructs the executor with N worker threads
    */
    explicit Executor(unsigned n = std::thread::hardware_concurrency());
    
    /**
    @brief destructs the executor 
    */
    ~Executor();

    /**
    @brief runs the framework once
    
    @param framework a tf::Framework object

    @return a std::shared_future to access the execution status of the framework
    */
    std::shared_future<void> run(Framework& framework);

    /**
    @brief runs the framework once and invoke a callback upon completion

    @param framework a tf::Framework object 
    @param callable a callable object to be invoked after this run

    @return a std::shared_future to access the execution status of the framework
    */
    template<typename C>
    std::shared_future<void> run(Framework& framework, C&& callable);

    /**
    @brief runs the framework for N times
    
    @param framework a tf::Framework object
    @param N number of runs

    @return a std::shared_future to access the execution status of the framework
    */
    std::shared_future<void> run_n(Framework& framework, size_t N);

    /**
    @brief runs the framework for N times and then invokes a callback

    @param framework a tf::Framework 
    @param N number of runs
    @param callable a callable object to be invoked after this run

    @return a std::shared_future to access the execution status of the framework
    */
    template<typename C>
    std::shared_future<void> run_n(Framework& framework, size_t N, C&& callable);

    /**
    @brief runs the framework multiple times until the predicate becomes true and 
           then invokes a callback

    @param framework a tf::Framework 
    @param predicate a boolean predicate to return true for stop

    @return a std::shared_future to access the execution status of the framework
    */
    template<typename P>
    std::shared_future<void> run_until(Framework& framework, P&& predicate);

    /**
    @brief runs the framework multiple times until the predicate becomes true and 
           then invokes the callback

    @param framework a tf::Framework 
    @param predicate a boolean predicate to return true for stop
    @param callable a callable object to be invoked after this run

    @return a std::shared_future to access the execution status of the framework
    */
    template<typename P, typename C>
    std::shared_future<void> run_until(Framework& framework, P&& predicate, C&& callable);

    /**
    @brief wait for all pending graphs to complete
    */
    void wait_for_all();

  private:
    
    std::list<Topology> _topologies;
    
    // scheduler field
    std::mutex _mutex;

    std::vector<Worker> _workers;
    std::vector<Notifier::Waiter> _waiters;
    std::vector<std::thread> _threads;

    WorkStealingQueue<Node*> _queue;
    
    std::atomic<size_t> _num_actives {0};
    std::atomic<size_t> _num_thieves {0};
    std::atomic<size_t> _num_idlers  {0};
    std::atomic<bool>   _done        {0};

    Notifier _notifier;
    
    std::unique_ptr<ExecutorObserverInterface> _observer;
    
    unsigned _find_victim(unsigned);

    PerThread& _per_thread() const;

    bool _wait_for_tasks(unsigned, std::optional<Node*>&);
    
    void _spawn(unsigned);
    void _exploit_task(unsigned, std::optional<Node*>&);
    void _explore_task(unsigned, std::optional<Node*>&);
    void _schedule(Node*);
    void _schedule(PassiveVector<Node*>&);
    void _invoke(Node*);
    void _init_module_node(Node*);
};

// Constructor
inline Executor::Executor(unsigned N) : 
  _workers   {N},
  _waiters   {N},
  _notifier  {_waiters} {
  _spawn(N);
}

// Destructor
inline Executor::~Executor() {
  
  // wait for all topologies to complete
  wait_for_all();
  
  // shut down the scheduler
  _done = true;
  _notifier.notify(true);
  
  for(auto& t : _threads){
    t.join();
  } 
}

// Function: _per_thread
inline Executor::PerThread& Executor::_per_thread() const {
  thread_local PerThread pt;
  return pt;
}

// Procedure: _spawn
inline void Executor::_spawn(unsigned N) {

  // Lock to synchronize all workers before creating _worker_maps
  for(unsigned i=0; i<N; ++i) {
    _threads.emplace_back([this, i] () -> void {

      PerThread& pt = _per_thread();  
      pt.pool = this;
      pt.worker_id = i;
    
      std::optional<Node*> t;
      
      // must use 1 as condition instead of !done
      while(1) {
        
        // execute the tasks.
        run_task:
        _exploit_task(i, t);

        // steal loop
        if(_explore_task(i, t); t) {
          goto run_task;
        }
        
        // wait for tasks
        if(_wait_for_tasks(i, t) == false) {
          break;
        }
      }
      
    });     
  }
}

// Function: _find_victim
inline unsigned Executor::_find_victim(unsigned thief) {
  
  /*unsigned l = 0;
  unsigned r = _workers.size() - 1;
  unsigned vtm = std::uniform_int_distribution<unsigned>{l, r}(
    _workers[thief].rdgen
  );

  // try to look for a task from other workers
  for(unsigned i=0; i<_workers.size(); ++i){

    if((thief == vtm && !_queue.empty()) ||
       (thief != vtm && !_workers[vtm].queue.empty())) {
      return vtm;
    }

    if(++vtm; vtm == _workers.size()) {
      vtm = 0;
    }
  }*/

  // try to look for a task from other workers
  for(unsigned vtm=0; vtm<_workers.size(); ++vtm){
    if((thief == vtm && !_queue.empty()) ||
       (thief != vtm && !_workers[vtm].queue.empty())) {
      return vtm;
    }
  }

  return _workers.size();
}

// Function: _explore_task
inline void Executor::_explore_task(unsigned thief, std::optional<Node*>& t) {
  
  //assert(_workers[thief].queue.empty());
  assert(!t);

  const unsigned l = 0;
  const unsigned r = _workers.size() - 1;

  steal_loop:

  size_t f = 0;
  size_t F = (_workers.size() + 1) << 1;
  size_t y = 0;

  ++_num_thieves;

  // explore
  while(!_done) {
  
    unsigned vtm = std::uniform_int_distribution<unsigned>{l, r}(
      _workers[thief].rdgen
    );
      
    t = (vtm == thief) ? _queue.steal() : _workers[vtm].queue.steal();

    if(t) {
      break;
    }

    if(f++ > F) {
      if(std::this_thread::yield(); y++ > 100) {
        break;
      }
    }

    /*if(auto vtm = _find_victim(thief); vtm != _workers.size()) {
      t = (vtm == thief) ? _queue.steal() : _workers[vtm].queue.steal();
      // successful thief
      if(t) {
        break;
      }
    }
    else {
      if(f++ > F) {
        if(std::this_thread::yield(); y++ > 100) {
          break;
        }
      }
    } */
  }
  
  // We need to ensure at least one thieve if there is an
  // active worker
  if(auto N = --_num_thieves; N == 0) {
  //if(auto N = _num_thieves.fetch_sub(1); N == 1) {
    if(t != std::nullopt) {
      _notifier.notify(false);
      return;
    }
    else if(_num_actives > 0) {
      goto steal_loop;
    }
  }
}

// Procedure: _exploit_task
inline void Executor::_exploit_task(unsigned i, std::optional<Node*>& t) {

  if(t) {

    auto& worker = _workers[i];

    if(++_num_actives; _num_thieves == 0) {
      _notifier.notify(false);
    }

    do {

      if(_observer) {
        _observer->on_entry(i);
      }

      //(*t)();
      _invoke(*t);

      if(_observer) {
        _observer->on_exit(i);
      }

      t = worker.queue.pop();

    } while(t);

    --_num_actives;
  }
}

// Function: _wait_for_tasks
inline bool Executor::_wait_for_tasks(unsigned me, std::optional<Node*>& t) {

  assert(!t);
  
  _notifier.prepare_wait(&_waiters[me]);
  
  //// check again.
  //if(!_queue.empty()) {
  //  t = _queue.steal();
  //  return true;
  //}

  //if(size_t I = ++_num_idlers; _done && I == _workers.size()) {
  //  _notifier.cancel_wait(&_waiters[me]);
  //  //if(_find_victim(me) != _workers.size()) {
  //  //  --_num_idlers;
  //  //  return true;
  //  //}
  //  _notifier.notify(true);
  //  return false;
  //}

  if(auto vtm = _find_victim(me); vtm != _workers.size()) {
    _notifier.cancel_wait(&_waiters[me]);
    t = (vtm == me) ? _queue.steal() : _workers[vtm].queue.steal();
    return true;
  }

  if(size_t I = ++_num_idlers; _done && I == _workers.size()) {
    _notifier.cancel_wait(&_waiters[me]);
    if(_find_victim(me) != _workers.size()) {
      --_num_idlers;
      return true;
    }
    _notifier.notify(true);
    return false;
  }
    
  // Now I really need to relinguish my self to others
  _notifier.commit_wait(&_waiters[me]);
  --_num_idlers;

  return true;
}

// Procedure: _schedule
// The main procedure to schedule a give task node.
// Each task node has two types of tasks - regular and subflow.
inline void Executor::_schedule(Node* node) {
  
  // module node need another initialization
  if(node->_module != nullptr && !node->is_spawned()) {
    _init_module_node(node);
  }
  
  //no worker thread available
  if(_workers.size() == 0){
    _invoke(node);
    return;
  }

  auto& pt = _per_thread();
  
  // caller is a worker to this pool
  if(pt.pool == this) {
    _workers[pt.worker_id].queue.push(node);
    return;
  }
  // other threads
  else {
    std::scoped_lock lock(_mutex);
    _queue.push(node);
  }

  _notifier.notify(false);
}

// Procedure: _schedule
// The main procedure to schedule a set of task nodes.
// Each task node has two types of tasks - regular and subflow.
inline void Executor::_schedule(PassiveVector<Node*>& nodes) {
  
  if(nodes.empty()) {
    return;
  }

  for(auto node : nodes) {
    if(node->_module != nullptr && !node->is_spawned()) {
      _init_module_node(node);
    }
  }

  //no worker thread available
  if(_workers.size() == 0){
    for(auto node: nodes){
      _invoke(node);
    }
    return;
  }

  auto& pt = _per_thread();

  if(pt.pool == this) {
    for(size_t i=0; i<nodes.size(); ++i) {
      _workers[pt.worker_id].queue.push(nodes[i]);
    }
    return;
  }
  
  {
    std::scoped_lock lock(_mutex);

    for(size_t k=0; k<nodes.size(); ++k) {
      _queue.push(nodes[k]);
    }
  }
  
  size_t N = std::max(size_t{1}, std::min(_num_idlers.load(), nodes.size()));

  if(N >= _workers.size()) {
    _notifier.notify(true);
  }
  else {
    for(size_t k=0; k<N; ++k) {
      _notifier.notify(false);
    }
  }
}

// Procedure: _init_module_node
inline void Executor::_init_module_node(Node* node) {

  node->_work = [node=node, this, tgt{PassiveVector<Node*>()}] () mutable {

    // second time to enter this context
    if(node->is_spawned()) {
      node->_dependents.resize(node->_dependents.size()-tgt.size());
      for(auto& t: tgt) {
        t->_successors.clear();
      }
      return ;
    }

    // first time to enter this context
    node->set_spawned();

    PassiveVector<Node*> src;

    for(auto &n: node->_module->_graph) {
      n._topology = node->_topology;
      if(n.num_dependents() == 0) {
        src.push_back(&n);
      }
      if(n.num_successors() == 0) {
        n.precede(*node);
        tgt.push_back(&n);
      }
    }

    _schedule(src);
  };
}

// Procedure: 
inline void Executor::_invoke(Node* node) {

  // Here we need to fetch the num_successors first to avoid the invalid memory
  // access caused by topology clear.
  const auto num_successors = node->num_successors();
  
  // regular node type
  // The default node work type. We only need to execute the callback if any.
  if(auto index=node->_work.index(); index == 0) {
    if(node->_module != nullptr) {
      bool first_time = !node->is_spawned();
      std::invoke(std::get<Node::StaticWork>(node->_work));
      if(first_time) {
        return ;
      }
    }
    else {
      if(auto &f = std::get<Node::StaticWork>(node->_work); f != nullptr){
        std::invoke(f);
      }
    }
  }
  // subflow node type 
  else {
    
    // Clear the subgraph before the task execution
    if(!node->is_spawned()) {
      node->_subgraph.emplace();
    }
   
    SubflowBuilder fb(*(node->_subgraph));

    std::invoke(std::get<Node::DynamicWork>(node->_work), fb);
    
    // Need to create a subflow if first time & subgraph is not empty 
    if(!node->is_spawned()) {
      node->set_spawned();
      if(!node->_subgraph->empty()) {
        // For storing the source nodes
        PassiveVector<Node*> src; 
        for(auto& n : *(node->_subgraph)) {
          n._topology = node->_topology;
          n.set_subtask();
          if(n.num_successors() == 0) {
            if(fb.detached()) {
              node->_topology->_num_sinks ++;
            }
            else {
              n.precede(*node);
            }
          }
          if(n.num_dependents() == 0) {
            src.push_back(&n);
          }
        }

        _schedule(src);

        if(!fb.detached()) {
          return;
        }
      }
    }
  } // End of DynamicWork -----------------------------------------------------
  
  // Recover the runtime change due to dynamic tasking except the target & spawn tasks 
  // This must be done before scheduling the successors, otherwise this might cause 
  // race condition on the _dependents
  //if(num_successors && !node->_subtask) {
  if(!node->is_subtask()) {
    // Only dynamic tasking needs to restore _dependents
    // TODO:
    if(node->_work.index() == 1 &&  !node->_subgraph->empty()) {
      while(!node->_dependents.empty() && node->_dependents.back()->is_subtask()) {
        node->_dependents.pop_back();
      }
    }
    node->_num_dependents = static_cast<int>(node->_dependents.size());
    node->unset_spawned();
  }

  // At this point, the node storage might be destructed.
  for(size_t i=0; i<num_successors; ++i) {
    if(--(node->_successors[i]->_num_dependents) == 0) {
      _schedule(node->_successors[i]);
    }
  }

  // A node without any successor should check the termination of topology
  if(num_successors == 0) {
    if(--(node->_topology->_num_sinks) == 0) {

      // This is the last executing node 
      bool is_framework = node->_topology->_handle.index() == 1;
      if(node->_topology->_work != nullptr) {
        std::invoke(node->_topology->_work);
      }
      if(!is_framework) {
        node->_topology->_promise.set_value();
      }
    }
  }
}

// Function: run
inline std::shared_future<void> Executor::run(Framework& f) {
  return run_n(f, 1, [](){});
}

// Function: run
template <typename C>
std::shared_future<void> Executor::run(Framework& f, C&& c) {
  static_assert(std::is_invocable<C>::value);
  return run_n(f, 1, std::forward<C>(c));
}

// Function: run_n
inline std::shared_future<void> Executor::run_n(Framework& f, size_t repeat) {
  return run_n(f, repeat, [](){});
}

// Function: run_n
template <typename C>
std::shared_future<void> Executor::run_n(Framework& f, size_t repeat, C&& c) {
  return run_until(f, [repeat]() mutable { return repeat-- == 0; }, std::forward<C>(c));
}

// Function: run_until    
template<typename P>
std::shared_future<void> Executor::run_until(Framework& f, P&& pred) {
  return run_until(f, std::forward<P>(pred), [](){});
}

// Function: run_until
template <typename P, typename C>
std::shared_future<void> Executor::run_until(Framework& f, P&& predicate, C&& c) {

  // Predicate must return a boolean value
  static_assert(std::is_invocable_v<C> && std::is_invocable_v<P>);

  if(std::invoke(predicate)) {
    return std::async(std::launch::deferred, [](){}).share();
  }
  
  // create a topology for this run
  auto &tpg = _topologies.emplace_back(f, std::forward<P>(predicate));

  // Iterative execution to avoid stack overflow
  if(_workers.size() == 0) {

    // Clear last execution data & Build precedence between nodes and target
    tpg._bind(f._graph);

    do {
      _schedule(tpg._sources);
      tpg._recover_num_sinks();
    } while(!std::invoke(tpg._predicate));

    std::invoke(c);
    tpg._promise.set_value();

    return tpg._future;
  }

  // Multi-threaded execution.
  std::scoped_lock lock(f._mtx);

  f._topologies.push_back(&tpg);

  bool run_now = (f._topologies.size() == 1);

  if(run_now) {
    tpg._bind(f._graph);
  }

  tpg._work = [&f, c=std::forward<C>(c), this] () mutable {
      
    // case 1: we still need to run the topology again
    if(!std::invoke(f._topologies.front()->_predicate)) {
      f._topologies.front()->_recover_num_sinks();
      _schedule(f._topologies.front()->_sources); 
    }
    // case 2: the final run of this topology
    else {

      std::invoke(c);

      f._mtx.lock();

      // If there is another run (interleave between lock)
      if(f._topologies.size() > 1) {

        // Set the promise
        f._topologies.front()->_promise.set_value();
        f._topologies.pop_front();
        f._topologies.front()->_bind(f._graph);
        f._mtx.unlock();
        _schedule(f._topologies.front()->_sources);
      }
      else {
        assert(f._topologies.size() == 1);
        // Need to back up the promise first here becuz framework might be 
        // destroy before taskflow leaves
        auto &p = f._topologies.front()->_promise; 
        f._topologies.pop_front();
        f._mtx.unlock();
       
        // We set the promise in the end in case framework leaves before taskflow
        p.set_value();
      }
    }
  };

  if(run_now) {
    _schedule(tpg._sources);
  }

  return tpg._future;
}

// Procedure: wait_for_all
inline void Executor::wait_for_all() {
  for(auto& t: _topologies){
    t._future.get();
  }
  _topologies.clear();
}

}  // end of namespace tf2 ----------------------------------------------------










