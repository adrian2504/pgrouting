/*PGR-GNU*****************************************************************

Copyright (c) 2015 pgRouting developers
Mail: project@pgrouting.org

Copyright (c) 2016 Andrea Nardelli
Mail: nrd.nardelli@gmail.com

------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

********************************************************************PGR-GNU*/

#pragma once

#ifdef __MINGW32__
#include <winsock2.h>
#include <windows.h>
#ifdef unlink
#undef unlink
#endif
#endif


#if 0
#include "./../../common/src/signalhandler.h"
#endif
#include "./../../common/src/pgr_types.h"

#include <cstdint>
#include <map>

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/push_relabel_max_flow.hpp>
#include <boost/graph/edmonds_karp_max_flow.hpp>
#include <boost/graph/boykov_kolmogorov_max_flow.hpp>
#include <boost/graph/max_cardinality_matching.hpp>
#include <boost/assert.hpp>

// user's functions
// for development

typedef boost::adjacency_list_traits<boost::vecS, boost::vecS, boost::directedS>
    Traits;
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS,
                              boost::property<boost::vertex_name_t, std::string,
                                              boost::property<boost::vertex_index_t,
                                                              int64_t,
                                                              boost::property<
                                                                  boost::vertex_color_t,
                                                                  boost::default_color_type,
                                                                  boost::property<
                                                                      boost::vertex_distance_t,
                                                                      int64_t,
                                                                      boost::property<
                                                                          boost::vertex_predecessor_t,
                                                                          Traits::edge_descriptor> > > > >,

                              boost::property<boost::edge_capacity_t,
                                              int64_t,
                                              boost::property<boost::edge_residual_capacity_t,
                                                              int64_t,
                                                              boost::property<
                                                                  boost::edge_reverse_t,
                                                                  Traits::edge_descriptor> > > >
    FlowGraph;
typedef boost::adjacency_list_traits<boost::vecS, boost::vecS, boost::undirectedS>
    UndirectedTraits;
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS,
                              boost::property<boost::vertex_name_t, std::string,
                                              boost::property<boost::vertex_index_t,
                                                              int64_t,
                                                              boost::property<
                                                                  boost::vertex_color_t,
                                                                  boost::default_color_type,
                                                                  boost::property<
                                                                      boost::vertex_distance_t,
                                                                      int64_t,
                                                                      boost::property<
                                                                          boost::vertex_predecessor_t,
                                                                          UndirectedTraits::edge_descriptor> > > > >,

                              boost::property<boost::edge_capacity_t,
                                              int64_t,
                                              boost::property<boost::edge_residual_capacity_t,
                                                              int64_t,
                                                              boost::property<
                                                                  boost::edge_reverse_t,
                                                                  UndirectedTraits::edge_descriptor> > > >
    UndirectedFlowGraph;
// Used internally for edge disjoint paths functionality


template<class G>
class PgrFlowGraph {
 public:
  G boost_graph;

  typedef typename boost::graph_traits<G>::vertex_descriptor V;
  typedef typename boost::graph_traits<G>::edge_descriptor E;
  typedef typename boost::graph_traits<G>::vertex_iterator V_it;
  typedef typename boost::graph_traits<G>::edge_iterator E_it;

  typename boost::property_map<G, boost::edge_capacity_t>::type capacity;
  typename boost::property_map<G, boost::edge_reverse_t>::type rev;
  typename boost::property_map<G, boost::edge_residual_capacity_t>::type
      residual_capacity;


  std::map<int64_t, V> id_to_V;
  std::map<V, int64_t> V_to_id;
  std::map<E, int64_t> E_to_id;

  V source_vertex;
  V sink_vertex;

  V get_boost_vertex(int64_t id) {
      return this->id_to_V.find(id)->second;
  }

  int64_t get_vertex_id(V v) {
      return this->V_to_id.find(v)->second;
  }

  int64_t get_edge_id(E e) {
      return this->E_to_id.find(e)->second;
  }

  int64_t push_relabel() {
      return boost::push_relabel_max_flow(this->boost_graph,
                                          this->source_vertex,
                                          this->sink_vertex);
  }

  int64_t edmonds_karp() {
      return boost::edmonds_karp_max_flow(this->boost_graph,
                                          this->source_vertex,
                                          this->sink_vertex);
  }

  int64_t boykov_kolmogorov() {
      size_t num_v = boost::num_vertices(this->boost_graph);
      std::vector<boost::default_color_type> color(num_v);
      std::vector<long> distance(num_v);
      return boost::boykov_kolmogorov_max_flow(this->boost_graph,
                                               this->source_vertex,
                                               this->sink_vertex);
  }

  void create_flow_graph(pgr_edge_t *data_edges,
                         size_t total_tuples,
                         const std::set<int64_t> &source_vertices,
                         const std::set<int64_t> &sink_vertices) {

      /* In multi source flow graphs, a super source is created connected to all sources with "infinite" capacity
       * The same applies for sinks.
       * To avoid code repetition, a supersource/sink is used even in the one to one signature.
       */


      std::set<int64_t> vertices;
      for (int64_t source: source_vertices) {
          vertices.insert(source);
      }
      for (int64_t sink: sink_vertices) {
          vertices.insert(sink);
      }
      for (size_t i = 0; i < total_tuples; ++i) {
          vertices.insert(data_edges[i].source);
          vertices.insert(data_edges[i].target);
      }
      for (int64_t id : vertices) {
          V v = add_vertex(this->boost_graph);
          this->id_to_V.insert(std::pair<int64_t, V>(id, v));
          this->V_to_id.insert(std::pair<V, int64_t>(v, id));
      }
      bool added;

      V supersource = add_vertex(this->boost_graph);
      for (int64_t source_id: source_vertices) {
          V source = this->id_to_V.find(source_id)->second;
          E e1, e1_rev;
          boost::tie(e1, added) =
              boost::add_edge(supersource, source, this->boost_graph);
          boost::tie(e1_rev, added) =
              boost::add_edge(source, supersource, this->boost_graph);
          this->capacity[e1] = 999999999;
          this->capacity[e1_rev] = 0;
          this->rev[e1] = e1_rev;
          this->rev[e1_rev] = e1;
      }

      V supersink = add_vertex(this->boost_graph);
      for (int64_t sink_id: sink_vertices) {
          V sink = this->id_to_V.find(sink_id)->second;
          E e1, e1_rev;
          boost::tie(e1, added) =
              boost::add_edge(sink, supersink, this->boost_graph);
          boost::tie(e1_rev, added) =
              boost::add_edge(supersink, sink, this->boost_graph);
          this->capacity[e1] = 999999999;
          this->capacity[e1_rev] = 0;
          this->rev[e1] = e1_rev;
          this->rev[e1_rev] = e1;
      }

      this->source_vertex = supersource;
      this->sink_vertex = supersink;

      this->capacity = get(boost::edge_capacity, this->boost_graph);
      this->rev = get(boost::edge_reverse, this->boost_graph);
      this->residual_capacity =
          get(boost::edge_residual_capacity, this->boost_graph);

      for (size_t i = 0; i < total_tuples; ++i) {
          V v1 = this->get_boost_vertex(data_edges[i].source);
          V v2 = this->get_boost_vertex(data_edges[i].target);
          if (data_edges[i].cost > 0) {
              E e1, e1_rev;
              boost::tie(e1, added) =
                  boost::add_edge(v1, v2, this->boost_graph);
              this->E_to_id.insert(std::pair<E, int64_t>(e1, data_edges[i].id));
              this->E_to_id.insert(std::pair<E, int64_t>(e1_rev, data_edges[i].id));
              boost::tie(e1_rev, added) =
                  boost::add_edge(v2, v1, this->boost_graph);
              this->capacity[e1] = (int64_t) data_edges[i].cost;
              this->capacity[e1_rev] = 0;
              this->rev[e1] = e1_rev;
              this->rev[e1_rev] = e1;
          }
          if (data_edges[i].reverse_cost > 0) {
              E e2, e2_rev;
              boost::tie(e2, added) =
                  boost::add_edge(v2, v1, this->boost_graph);
              boost::tie(e2_rev, added) =
                  boost::add_edge(v1, v2, this->boost_graph);
              this->E_to_id.insert(std::pair<E, int64_t>(e2, data_edges[i].id));
              this->E_to_id.insert(std::pair<E, int64_t>(e2_rev, data_edges[i].id));
              this->capacity[e2] = (int64_t) data_edges[i].reverse_cost;
              this->capacity[e2_rev] = 0;
              this->rev[e2] = e2_rev;
              this->rev[e2_rev] = e2;
          }
      }
  }

  void get_flow_edges(std::vector<pgr_flow_t> &flow_edges) {
      int64_t id = 1;
      E_it e, e_end;
      for (boost::tie(e, e_end) = boost::edges(this->boost_graph); e != e_end;
           ++e) {
          // A supersource/supersink is used internally
          if (((this->capacity[*e] - this->residual_capacity[*e]) > 0) &&
              ((*e).m_source != this->source_vertex) &&
              ((*e).m_target != this->sink_vertex)) {
              pgr_flow_t edge;
              edge.id = id++;
              edge.edge = this->get_edge_id(*e);
              edge.source = this->get_vertex_id((*e).m_source);
              edge.target = this->get_vertex_id((*e).m_target);
              edge.flow = this->capacity[*e] - this->residual_capacity[*e];
              edge.residual_capacity = this->residual_capacity[*e];
              flow_edges.push_back(edge);
          }
      }
  }
};