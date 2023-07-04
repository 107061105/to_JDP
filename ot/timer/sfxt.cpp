#include <ot/timer/sfxt.hpp>
#include <ot/timer/timer.hpp>

namespace ot {

// Constructor
SfxtCache::SfxtCache(Split el, size_t S, size_t T) : 
  _el   {el}, 
  _S    {S}, 
  _T    {T},
  _pins {std::move(__pins)} {

  resize_to_fit(std::max(S, T) + 1, __tree, __link, __dist, __spfa);

  // debug
  //for(const auto& i : __tree) assert(!i);
  //for(const auto& i : __dist) assert(!i);
  //for(const auto& i : __spfa) assert(!i);
}

// Move constructor
SfxtCache::SfxtCache(SfxtCache&& rhs) : 
  _el   {rhs._el},
  _S    {rhs._S},
  _T    {rhs._T},
  _pins {std::move(rhs._pins)},
  _srcs {std::move(rhs._srcs)} {
}

// Destructor
SfxtCache::~SfxtCache() {

  __dist[_S].reset();
  __tree[_S].reset();
  __link[_S].reset();
  __spfa[_S].reset();

  for(const auto& p : _pins) {
    __dist[p].reset();
    __tree[p].reset();
    __link[p].reset();
    __spfa[p].reset();
  }

  _pins.clear();
  __pins = std::move(_pins);
}
        


// ----------------------------------------------------------------------------

// Procedure: _topologize
void Timer::_topologize(SfxtCache& sfxt, size_t v, const PathGuide* pg) const {

  sfxt.__spfa[v] = true;

  auto [pin, vrf] = _decode_pin(v);

  // Stop at the data source
  if(!pin->is_datapath_source()) {
    for(auto arc : pin->_fanin) {
      // FOR_EACH_RF_IF(urf, arc->_delay[sfxt._el][urf][vrf]) {
      FOR_EACH_RF_IF(urf, arc->_s_delay[sfxt._el][urf][vrf]) {
        auto u = _encode_pin(arc->_from, urf);

        //if sftx is linked with guide, but node is not in range, ignore it 
        if(pg != nullptr && !_in_tail(pg->id, v, u)){ 
          continue;
        }

        if(!sfxt.__spfa[u]) {
          _topologize(sfxt, u, pg);
        }
      }
    }
  }
  
  sfxt._pins.push_back(v);
}

// Procedure: _spdp
void Timer::_spdp(SfxtCache& sfxt, const PathGuide* pg) const {
  
  assert(sfxt._pins.empty());
  
  if(pg != nullptr) {
    sfxt._pins = pg->pins;
  }

  _topologize(sfxt, sfxt._T, pg);
  
  assert(!sfxt._pins.empty());

  auto el = sfxt._el;

  for(auto itr = sfxt._pins.rbegin(); itr != sfxt._pins.rend(); ++itr) {

    auto v = *itr;
    auto [pin, vrf] = _decode_pin(v);

    //assert(sfxt.__dist[v]);
    //_pins obey topological order but some pin may not connect to _T
    if(!sfxt.__dist[v]){
      continue;
    }
    
    // Stop at the data source
    if(pin->is_datapath_source()) {
      sfxt._srcs.try_emplace(v, std::nullopt);
      continue;
    }

    // Relax on fanin
    for(auto arc : pin->_fanin) {
      // FOR_EACH_RF_IF(urf, arc->_delay[el][urf][vrf]) {
      FOR_EACH_RF_IF(urf, arc->_s_delay[el][urf][vrf]) {
        auto u = _encode_pin(arc->_from, urf);
        //if sftx is linked with guide, but node is not in range, ignore it 
        if(pg != nullptr && !_is_from_inbound(*pg, v, u)){
          continue;
        }
        // yclo 
        // auto d = (el == MIN) ? *arc->_delay[el][urf][vrf] : -(*arc->_delay[el][urf][vrf]);
        float d = (el == MIN) ? (*arc->_s_delay[el][urf][vrf]).nominal() : -(*arc->_s_delay[el][urf][vrf]).nominal();

        sfxt._relax(u, v, _encode_arc(*arc, urf, vrf), d);
      }
    }
  }
}

// Procedure: _spfa
// Perform shortest path fast algorithm (SPFA) to build up the suffix tree.
void Timer::_spfa(SfxtCache& sfxt) const {
  
  auto el = sfxt._el;
  
  std::queue<size_t> queue;
  queue.push(sfxt._T);
  sfxt.__spfa[sfxt._T] = true;

  while(!queue.empty()) {

    auto v = queue.front();
    queue.pop();
    sfxt.__spfa[v] = false;
    sfxt._pins.push_back(v);

    auto [pin, vrf] = _decode_pin(v);
    
    // Stop at the data source
    if(pin->is_datapath_source()) {
      sfxt._srcs.try_emplace(v, std::nullopt);
      continue;
    }

    // Relax on fanin
    for(auto arc : pin->_fanin) {
      // FOR_EACH_RF_IF(urf, arc->_delay[el][urf][vrf]) {
      FOR_EACH_RF_IF(urf, arc->_s_delay[el][urf][vrf]) {
        auto u = _encode_pin(arc->_from, urf);
        // yclo
        // auto d = (el == MIN) ? *arc->_delay[el][urf][vrf] : -(*arc->_delay[el][urf][vrf]);
        float d = (el == MIN) ? (*arc->_s_delay[el][urf][vrf]).nominal() : -(*arc->_s_delay[el][urf][vrf]).nominal();
        
        if(sfxt._relax(u, v, _encode_arc(*arc, urf, vrf), d)) {
          if(!sfxt.__spfa[u] || *sfxt.__spfa[u] == false) {
            queue.push(u);
            sfxt.__spfa[u] = true;
          }
        }
      }
    }
  }
}

// Function: _sfxt_cache
// Find the suffix tree rooted at the primary output po.
SfxtCache Timer::_sfxt_cache(const PrimaryOutput& po, Split el, Tran rf, const PathGuide* pg) const {
  
  assert(po._rat[el][rf]);

  // create a cache 
  auto S = _idx2pin.size() << 1;
  auto v = _encode_pin(po._pin, rf);

  SfxtCache sfxt(el, S, v);

  // start at the root
  assert(!sfxt.__dist[v]);
  sfxt.__dist[v] = (el == MIN) ? -(*po._rat[el][rf]) : *po._rat[el][rf];

  // shortest path dynamic programming
  _spdp(sfxt, pg);

  // shortest path fast algorithm
  //_spfa(sfxt);
  
  // relax sources
  for(auto& [s, v] : sfxt._srcs) {
    if(v = _sfxt_offset(sfxt, s); v) {
      sfxt._relax(S, s, std::nullopt, *v);
    }
  }

  return sfxt;
}

// Function: _sfxt_cache
// Find the suffix tree rooted at the test
SfxtCache Timer::_sfxt_cache(const Test& test, Split el, Tran rf, const PathGuide* pg) const {

  assert(test._rat[el][rf]);

  // create a cache
  auto S = _idx2pin.size() << 1;
  auto v = _encode_pin(test._arc._to, rf);
  SfxtCache sfxt(el, S, v);

  // Start at the D pin and perform SPFA all the way to the sources of data paths.
  assert(!sfxt.__dist[v]);
  sfxt.__dist[v] = (el == MIN) ? -(*test._rat[el][rf]) : *test._rat[el][rf];
  
  // shortest path dynamic programming
  _spdp(sfxt, pg);
  
  // shortest path fast algorithm
  //_spfa(sfxt);

  // relaxation from the sources
  if(_cppr_analysis) {
    auto cppr = _cppr_cache(test, el, rf);
    for(auto& [s, v] : sfxt._srcs) {
      auto [pin, srf] = _decode_pin(s);
      if(v = _cppr_offset(cppr, *pin, el, srf); v) {
        sfxt._relax(S, s, std::nullopt, *v);
      }
    }
  }
  else {
    for(auto& [s, v] : sfxt._srcs) {
      if(v = _sfxt_offset(sfxt, s); v) {
        sfxt._relax(S, s, std::nullopt, *v);
      }
    }
  }

  return sfxt;
}

// Function: _sfxt_cache
SfxtCache Timer::_sfxt_cache(const Endpoint& ept, const PathGuide *pg) const {
  return std::visit([this, &ept, pg] (auto&& handle) {
    return _sfxt_cache(*handle, ept._el, ept._rf, pg);
  }, ept._handle);
}

// Function: _sfxt_offset
std::optional<float> Timer::_sfxt_offset(const SfxtCache& sfxt, size_t v) const {

  auto [pin, rf] = _decode_pin(v);
  
  if(auto at = pin->_at[sfxt._el][rf]; at) {
    //return sfxt._el == MIN ? *at : -*at;

    // In tau18 contest, ideal_clock is used
    if(_ideal_clock && pin->primary_input() == nullptr) {
      return 0.0f;
    }
    // yclo
    return sfxt._el == MIN ? (*at).dist.nominal() : -(*at).dist.nominal();

  }
  else {
    return std::nullopt;
  }
}

};  // end of namespace ot. -----------------------------------------------------------------------



