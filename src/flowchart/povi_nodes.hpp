#include "geoflow.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include "imgui.h"
#include "gloo.h"
#include "app_povi.h"
#include "imgui_color_gradient.h"
#include <algorithm>
#include <random>

using namespace geoflow;
struct ColorMap {
  std::weak_ptr<Uniform> u_valmax, u_valmin;
  bool is_gradient=false;
  std::weak_ptr<Texture1D> tex;
  std::unordered_map<int,int> mapping;
};
class ColorMapperNode:public Node {
  std::shared_ptr<Texture1D> texture;
  std::array<float,256*3> colors;
  unsigned char tex[256*3];
  ColorMap colormap;

  size_t n_bins=100;
  float minval, maxval, bin_width;
  std::map<int,int> value_counts;

  public:
  ColorMapperNode(NodeManager& manager):Node(manager, "ColorMapper") {
    add_input("values", TT_vec1i);
    add_output("colormap", TT_colmap);
    texture = std::make_shared<Texture1D>();
    texture->set_interpolation_nearest();
    colors.fill(0);
  }

  void update_texture(){
    int i=0;
    for(auto& c : colors){
        tex[i] = c * 255;
        i++;
    }
    if (texture->is_initialised())
      texture->set_data(tex, 256);
  }

  void count_values() {
    auto data = std::any_cast<vec1i>(get_value("values"));
    for(auto& val : data) {
      value_counts[val]++;
    }
  }

  void on_push(InputTerminal& t) {
    if(inputTerminals["values"].get() == &t) {
      count_values();
    }
  }

  void on_connect(OutputTerminal& t) {
    if(outputTerminals["colormap"].get() == &t) {
      update_texture();
    }
  }

  void gui(){
    // ImGui::PlotHistogram("Histogram", histogram.data(), histogram.size(), 0, NULL, 0.0f, 1.0f, ImVec2(200,80));
    int i=0;
    if(ImGui::Button("Randomize colors")) {
      std::random_device rd;  //Will be used to obtain a seed for the random number engine
      std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
      std::uniform_real_distribution<> dis(0.0, 1.0);
      for(int i=0; i<colors.size()/3; i++) {
        ImGui::ColorConvertHSVtoRGB((float)dis(gen), 1.0, 1.0, colors[(i*3)+0], colors[(i*3)+1], colors[(i*3)+2]);
      }
      update_texture();
    }
    for (auto& cv : value_counts) {
      ImGui::PushID(i);
      if(ImGui::ColorEdit3("MyColor##3", (float*)&colors[i*3], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
        update_texture();
      ImGui::SameLine();
      ImGui::Text("%d [%d]", cv.first, cv.second);
      ImGui::PopID();
      if(++i==256) break;
    }
  }

  void process(){
    int i=0;
    for (auto& cv : value_counts){
      colormap.mapping[cv.first] = i++;
    }
    colormap.tex = texture;
    set_value("colormap", colormap);
  }
};

class GradientMapperNode:public Node {
  std::shared_ptr<Texture1D> texture;
  std::shared_ptr<Uniform1f> u_maxval, u_minval;
  unsigned char tex[256*3];
  ColorMap colormap;

  ImGradient gradient;
  ImGradientMark* draggingMark = nullptr;
  ImGradientMark* selectedMark = nullptr;

  size_t n_bins=100;
  float minval, maxval, bin_width;
  vec1f histogram;

  public:
  GradientMapperNode(NodeManager& manager):Node(manager, "GradientMapper") {
    add_input("values", TT_vec1f);
    add_output("colormap", TT_colmap);
    texture = std::make_shared<Texture1D>();
    u_maxval = std::make_shared<Uniform1f>("u_value_max");
    u_minval = std::make_shared<Uniform1f>("u_value_min");
  }

  void update_texture(){
    gradient.getTexture(tex);
    if (texture->is_initialised())
      texture->set_data(tex, 256);
  }

  void compute_histogram(float min, float max) {
    auto data = std::any_cast<vec1f>(get_value("values"));
    histogram.clear();
    histogram.resize(n_bins,0);
    bin_width = (max-min)/(n_bins-1);
    for(auto& val : data) {
      auto bin = std::floor((val-minval)/bin_width);
      histogram[bin]++;
    }
    auto max_bin_count = *std::max_element(histogram.begin(), histogram.end());
    for(auto &bin : histogram) bin /= max_bin_count;
  }

  void on_push(InputTerminal& t) {
    if(inputTerminals["values"].get() == &t) {
      auto& d = std::any_cast<vec1f&>(t.cdata);
      minval = *std::min_element(d.begin(), d.end());
      maxval = *std::max_element(d.begin(), d.end());
      compute_histogram(minval, maxval);
      u_maxval->set_value(maxval);
      u_minval->set_value(minval);
    }
  }

  void gui(){
    ImGui::DragFloatRange2("range", &u_minval->get_value(), &u_maxval->get_value(), 0.1f, minval, maxval, "Min: %.2f", "Max: %.2f");
    if(inputTerminals["values"]->cdata.has_value())
      if(ImGui::Button("Rescale histogram")){
        compute_histogram(u_minval->get_value(), u_maxval->get_value());
      }
    ImGui::PlotHistogram("Histogram", histogram.data(), histogram.size(), 0, NULL, 0.0f, 1.0f, ImVec2(200,80));
    if(ImGui::GradientEditor("Colormap", &gradient, draggingMark, selectedMark, ImVec2(200,80))){
      update_texture();
    }
  }

  void process(){
    update_texture();
    colormap.tex = texture;
    colormap.is_gradient = true;
    colormap.u_valmax = u_maxval;
    colormap.u_valmin = u_minval;
    set_value("colormap", colormap);
  }
};

class PoviPainterNode:public Node {
  std::shared_ptr<Painter> painter;
  std::weak_ptr<poviApp> pv_app;
  
  public:
  std::string name = "mypainter";
  PoviPainterNode(NodeManager& manager):Node(manager, "PoviPainter") {
    painter = std::make_shared<Painter>();
    // painter->set_attribute("position", nullptr, 0, {3});
    // painter->set_attribute("value", nullptr, 0, {1});
    painter->attach_shader("basic.vert");
    painter->attach_shader("basic.frag");
    painter->set_drawmode(GL_TRIANGLES);
    // a.add_painter(painter, "mypainter");
    add_input("vertices", TT_vec3f);
    add_input("colormap", TT_colmap);
    add_input("values", TT_vec1f);
    add_input("identifiers", TT_vec1i);
  }
  ~PoviPainterNode(){
    // note: this assumes we have only attached this painter to one poviapp
    if (auto a = pv_app.lock()) {
      std::cout << "remove painter\n";
      a->remove_painter(painter);
    } else std::cout << "remove painter failed\n";
  }

  void add_to(poviApp& a, std::string name) {
    a.add_painter(painter, name);
    pv_app = a.get_ptr();
  }

  void map_identifiers(){
    if (get_value("identifiers").has_value() && get_value("colormap").has_value()) {
      auto cmap = std::any_cast<ColorMap>(get_value("colormap"));
      if (cmap.is_gradient) return;
      auto values = std::any_cast<vec1i>(get_value("identifiers"));
      vec1f mapped;
      for(auto& v : values){
        mapped.push_back(float(cmap.mapping[v])/256);
      }
      painter->set_attribute("identifier", mapped.data(), mapped.size(), {1});
    }
  }

  void on_push(InputTerminal& t) {
    // auto& d = std::any_cast<std::vector<float>&>(t.cdata);
    if(t.cdata.has_value()) {
      if(inputTerminals["vertices"].get() == &t) {
        auto& d = std::any_cast<vec3f&>(t.cdata);
        painter->set_attribute("position", d[0].data(), d.size()*3, {3});
      } else if(inputTerminals["values"].get() == &t) {
        auto& d = std::any_cast<vec1f&>(t.cdata);
        painter->set_attribute("value", d.data(), d.size(), {1});
      } else if(inputTerminals["identifiers"].get() == &t) {
        map_identifiers();
      } else if(inputTerminals["colormap"].get() == &t) {
        auto& cmap = std::any_cast<ColorMap&>(t.cdata);
        painter->clear_uniforms();
        if(cmap.is_gradient) {
          painter->add_uniform(cmap.u_valmax);
          painter->add_uniform(cmap.u_valmin);
        } else {
          map_identifiers();
        }
        painter->set_texture(cmap.tex);
      }
    }
  }
  void on_clear(InputTerminal& t) {
    // clear attributes...
    // painter->set_attribute("position", nullptr, 0, {3}); // put empty array
     if(inputTerminals["vertices"].get() == &t) {
        painter->clear_attribute("position");
      } else if(inputTerminals["values"].get() == &t) {
        painter->clear_attribute("value");
      } else if(inputTerminals["colormap"].get() == &t) {
        painter->clear_uniforms();
        painter->remove_texture();
      }
  }

  void gui(){
    painter->gui();
    // type: points, lines, triangles
    // fp_painter->attach_shader("basic.vert");
    // fp_painter->attach_shader("basic.frag");
    // fp_painter->set_drawmode(GL_LINE_STRIP);
  }
};

class TriangleNode:public Node {
  public:
  vec3f vertices = {
    {10.5f, 9.5f, 0.0f}, 
    {9.5f, 9.5f, 0.0f},
    {10.0f,  10.5f, 0.0f}
  };
  vec3f colors = {
    {1.0f, 0.0f, 0.0f}, 
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}
  };
  vec1f attrf = {1.0,5.5,10.0};
  vec1i attri = {1,42,42};

  TriangleNode(NodeManager& manager):Node(manager, "Triangle") {
    add_output("vertices", TT_vec3f);
    add_output("colors", TT_vec3f);
    add_output("attrf", TT_vec1f);
    add_output("attri", TT_vec1i);
  }

  void gui(){
    ImGui::ColorEdit3("col1", colors[0].data());
    ImGui::ColorEdit3("col2", colors[1].data());
    ImGui::ColorEdit3("col3", colors[2].data());
  }

  void process(){
    set_value("vertices", vertices);
    set_value("colors", colors);
    set_value("attrf", attrf);
    set_value("attrf", attrf);
    set_value("attri", attri);
  }
};