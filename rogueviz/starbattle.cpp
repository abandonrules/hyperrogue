// Hyperbolic Rogue -- sumotron puzzle
// Copyright (C) 2011-2019 Zeno Rogue, see 'hyper.cpp' for details

/** \file sumotron.cpp
 *  \brief dual geometry puzzle generator
 */

#include "../hyper.h"

namespace hr {

EX namespace sumotron {

bool in = false;

bool dialog_shown = false;

bool view_errors = false;
bool view_lines = true;
bool view_regions = false;

int stars_per_line = 1;
int stars_per_region = 1;

struct stardata {
  color_t region;
  bool starred;
  bool illegal;
  };

map<color_t, int> per_region;

map<cell*, stardata> sdata;

struct starwalker : cellwalker {
  int flip;
  starwalker(cell *c, int i, int f) : cellwalker(c, i), flip(f) {}
  friend bool operator < (const starwalker& a, const starwalker& b) {
    return tie((const cellwalker&) a, a.flip) < tie((const cellwalker&) b, b.flip);
    }
  };

using line_t = vector<starwalker>;
vector<line_t> lines;

starwalker rev(starwalker sw) {
  sw += (sw.at->type + sw.flip) / 2;
  if(sw.at->type & 1) sw.flip = !sw.flip;
  return sw;
  }

starwalker change_flip(starwalker sw) {
  sw.flip = !sw.flip;
  return sw;
  }

enum ePenMode {
  pmSolve,
  pmDraw,
  pmCreate,
  pmWalls
  };

void create_lines() {
  set<starwalker> used;
  lines.clear();
  
  for(int it: {0,1})
  for(auto c: sdata)
  for(int i=0; i<c.first->type; i++)
  for(int f=0; f<2; f++) {
    starwalker sw(c.first, i, f);
    /* with it=0, find lines which just have started */
    if(it == 0 && sdata.count((rev(sw) + wstep).at)) continue;
    if(used.count(sw) || used.count(rev(sw))) continue;
    
    starwalker sw0 = sw;
    vector<starwalker> line;
    bool even = true;
    while(true) {
      if(sw.at->type & 1) even = false;
      line.push_back(sw);
      used.insert(sw);
      sw += wstep;
      sw = rev(sw);
      if(!sdata.count(sw.at)) break;
      if(sw == sw0) break;
      }
    lines.push_back(line);
    if(even) for(auto li: line) used.insert(change_flip(li));
    }
  }

void init_starbattle() {
  for(cell *c: currentmap->allcells()) 
    sdata[c] = stardata{color_t(0xFFFFFF), false, false};
  create_lines();
  }

ePenMode pen = pmSolve;

bool draw_star(cell *c, const shiftmatrix& V) {
  if(!in || !sdata.count(c)) return false;
    
  auto& sd = sdata[c];
  c->wall = waNone;
  c->landparam = view_regions ? sd.region : 0xFFFFFF;
  c->item = itNone;
  
  if(view_errors && per_region[sd.region] != stars_per_region)
    c->landparam = (per_region[sd.region] > stars_per_region) ? 0xFF4040 : 0x4040FF;
  
  int prec = sphere ? 3 : 1;
  prec += vid.linequality;

  forCellIdCM(c1, i, c) {
    if(!sdata.count(c1)) continue;
    auto& sd1 = sdata[c1];
    vid.linewidth *= 16;
    if(sd1.region != sd.region) 
      gridline(V, get_corner_position(c, i), get_corner_position(c, gmod(i+1, c->type)), 0xFF, 4);
    vid.linewidth /= 16;
    }
  
  if(sd.starred)
    queuepoly(V, cgi.shStar, 0xFF);

  if(sd.illegal) {
    dynamicval d(poly_outline, 0xFF0000FF);
    queuepoly(V, cgi.shPirateX, 0xFF0000FF);
    }
  
  if(pen == pmWalls && c == currentmap->gamestart())
    queuepoly(V, cgi.shTriangle, 0xC0FF);
  
  return false;
  }

color_t new_region() {
  again:
  color_t ng = hrand(0x1000000) | 0xC0C0C0;
  for(auto& sd: sdata) if(sd.second.region == ng) goto again;
  return ng;
  }
  
color_t extregion;

vector<color_t> linecolors = {
  0xFF000040, 0x0000FF40, 0x00FF0040, 
  0xFFFF0040, 0xFF00FF40, 0x00FFFF40, 
  0xFFD50040, 0x40806040
  };

void draw_line(line_t& li, color_t col) {
  vid.linewidth *= 10;
  for(auto& lii: li) {
    for(const shiftmatrix& V: current_display->all_drawn_copies[lii.at])
      if(sdata.count(lii.peek()))
        queueline(V*C0, V*currentmap->adj(lii.at, lii.spin)*C0, col);
    }
  vid.linewidth /= 10;
  }
  
void starbattle_puzzle() {

  clearMessages();

  if(currentmap->gamestart()->wall == waSea) 
    init_starbattle();
  
  getcstat = '-';
  cmode = 0;
  if(dialog_shown) cmode = sm::SIDE | sm::DIALOG_STRICT_X | sm::MAYDARK;
  gamescreen(0);
  
  if(view_lines && mouseover) {
    int lci = 0;
    initquickqueue();
    int dir = neighborId(mouseover, mouseover2);
    for(auto& li: lines) {
      bool has = false;
      for(auto& lii: li) if(lii.at == mouseover && (dir == -1 || lii.spin == dir || rev(lii).spin == dir)) has = true;
      if(has) {
        draw_line(li, linecolors[lci]);
        lci++;
        lci %= isize(linecolors);
        }        
      }
    quickqueue();
    }
  
  if(view_errors) {
    initquickqueue();
    per_region.clear();
    for(auto& sd: sdata) if(sd.second.starred) per_region[sd.second.region]++;

    for(auto& sd: sdata) if(sd.second.starred) 
      for(cell *c2: adj_minefield_cells(sd.first))
        if(sdata.count(c2) && sdata[c2].starred) {
          vid.linewidth *= 16;
          transmatrix rel = currentmap->relative_matrix(c2, sd.first, C0);
          for(const shiftmatrix& V: current_display->all_drawn_copies[sd.first])
            queueline(V * C0, V * rel * C0, 0xFF000060, 2, PPR::LINE);
          vid.linewidth /= 16;
          }
    for(auto& li: lines) {
      int sc = 0;
      for(auto& lii: li) if(sdata[lii.at].starred) sc++;
      if(sc < stars_per_line)
        draw_line(li, 0x0000FF80);
      if(sc > stars_per_line)
        draw_line(li, 0xFF000080);
      }
    quickqueue();
    }

  dialog::init("Star Battle", iinf[itPalace].color, 150, 100);

  dialog::addSelItem("stars per line", its(stars_per_line), 'l');
  dialog::add_action([] { 
    dialog::editNumber(stars_per_line, 0, 4, 1, 1, "", ""); 
    });

  dialog::addSelItem("stars per region", its(stars_per_region), 'g');
  dialog::add_action([] { 
    dialog::editNumber(stars_per_region, 0, 4, 1, 1, "", ""); 
    });
  
  dialog::addBoolItem("view errors", view_errors, 'e');
  dialog::add_action([] { view_errors = !view_errors; view_lines = view_regions = false; });
  dialog::addBoolItem("view lines", view_lines, 'l');
  dialog::add_action([] { view_lines = !view_lines; view_errors = false; });
  dialog::addBoolItem("view regions", view_regions, 'r');
  dialog::add_action([] { view_regions = !view_regions; view_errors = false; });
  
  dialog::addBreak(100);

  dialog::addBoolItem("solve mode", pen == pmSolve, 's');
  dialog::add_action([] { pen = pmSolve; });

  dialog::addBoolItem("free draw mode", pen == pmDraw, 'p');
  dialog::add_action([] { pen = pmDraw; });

  dialog::addBoolItem("create mode", pen == pmCreate, 'e');
  dialog::add_action([] { pen = pmCreate; });

  dialog::addBoolItem("area mode", pen == pmWalls, 'x');
  dialog::add_action([] { pen = pmWalls; });
  
  if(pen == pmSolve) dialog::addInfo("LMB = star, RMB = no star");
  if(pen == pmDraw) dialog::addInfo("LMB = draw, RMB = erase");
  if(pen == pmCreate) dialog::addInfo("LMB = new region, RMB = extend");
  if(pen == pmWalls) dialog::addInfo("LMB = add, RMB = remove");
  
  dialog::addBreak(100);
  
  dialog::addItem("save this puzzle", 'S');
  dialog::add_action([] { 
    mapstream::saveMap("starbattle.lev");
    #if ISWEB
    offer_download("starbattle.lev", "mime/type");
    #endif
    });

  dialog::addItem("settings", 'X');
  dialog::add_action_push(showSettings);

  mine_adjacency_rule = true;
  
  dialog::addItem("new geometry", 'G');
  dialog::add_action(runGeometryExperiments);

  dialog::addItem("screenshot", 'Y');
  dialog::add_action_push(shot::menu);

  dialog::addItem("load a puzzle", 'L');
  dialog::add_action([] { 
    #if ISWEB
    offer_choose_file([] {
      mapstream::loadMap("data.txt");
      });
    #else
    mapstream::loadMap("starbattle.lev");
    #endif
    mapeditor::drawplayer = false;
    });

  dialog::addItem("hide menu", 'v');
  dialog::add_action([] { dialog_shown = false; });
  
  if(dialog_shown) {
    dialog::display();
    }
  else {
    displayButton(vid.xres-8, vid.yres-vid.fsize, XLAT("(v) menu"), 'v', 16);
    dialog::add_key_action('v', [] { dialog_shown = true; });
    }
  
  keyhandler = [] (int sym, int uni) {
    handlePanning(sym, uni);
    dialog::handleNavigation(sym, uni);
    if(sym == SDLK_F1 && mouseover && !holdmouse) {
      cell *c = mouseover;
      if(pen == pmSolve) {
        if(!sdata.count(c)) return;
        auto& sd = sdata[c];
        sd.illegal = !sd.illegal;
        if(sd.illegal) sd.starred = false;
        holdmouse = true;
        extregion = sd.illegal ? 3 : 2;
        }
      if(pen == pmCreate) {
        if(!sdata.count(c)) return;
        auto& sd = sdata[c];
        extregion = sd.region;
        }
      if(pen == pmDraw) { 
        mapeditor::dt_erase(mouseh);
        }
      if(pen == pmWalls) {
        if(!sdata.count(c)) return;
        if(c == currentmap->gamestart()) return;
        sdata.erase(c);
        c->wall = waSea;
        c->land = laNone;
        create_lines();
        holdmouse = true;
        extregion = 0;
        }
      }
    if(sym == '-' && mouseover && !holdmouse) {
      cell *c = mouseover;
      if(pen == pmSolve) {
        if(!sdata.count(c)) return;
        auto& sd = sdata[c];
        sd.starred = !sd.starred;
        if(sd.starred) sd.illegal = false;
        holdmouse = true;
        extregion = sd.starred ? 1 : 0;
        }
      if(pen == pmCreate) {
        if(!sdata.count(c)) return;
        auto& sd = sdata[c];
        sd.region = extregion = new_region();
        }
      if(pen == pmDraw) { 
        mapeditor::dt_add_free(mouseh);
        holdmouse = true;
        }
      if(pen == pmWalls) {
        if(sdata.count(c)) return;
        c->wall = waNone;
        c->land = laCanvas;
        sdata[c] = stardata{color_t(0xFFFFFF), false, false};
        create_lines();
        holdmouse = true;
        extregion = 1;
        }
      }
    if(mouseover && mousepressed) {
      cell *c = mouseover;
      if(pen == pmSolve) {
        if(!sdata.count(c)) return;
        auto& sd = sdata[c];
        if(extregion >= 2)
          sd.illegal = extregion == 3;
        else
          sd.starred = extregion == 1;
        }
      if(pen == pmWalls && extregion == 1) {
        if(sdata.count(c)) return;
        c->wall = waNone;
        c->land = laCanvas;
        sdata[c] = stardata{color_t(0xFFFFFF), false, false};
        create_lines();
        }
      if(pen == pmWalls && extregion == 0) {
        if(!sdata.count(c)) return;
        if(c == currentmap->gamestart()) return;
        sdata.erase(c);
        c->wall = waSea;
        c->land = laNone;
        create_lines();
        }
      if(pen == pmCreate) {
        if(!sdata.count(c)) return;
        auto& sd = sdata[c];
        sd.region = extregion;
        }
      if(pen == pmDraw && holdmouse) { 
        mapeditor::dt_add_free(mouseh);
        }
      }
    if(pen == pmDraw && sym == PSEUDOKEY_RELEASE) {
      mapeditor::dt_finish();
      }
    };
  }

void launch() {  
  /* setup */
  stop_game();
  specialland = firstland = laCanvas;
  canvas_default_wall = waSea;
  start_game();
  screens[0] = starbattle_puzzle;

  in = true;
  mapeditor::dtcolor = 0xC000FF;
  mapeditor::dtwidth = .05;
  
  vid.linequality = 2;
  patterns::whichShape = '9';
  vid.use_smart_range = 2;
  vid.smart_range_detail = 2;
  
  showstartmenu = false;
  mapeditor::drawplayer = false;
  }

#if CAP_COMMANDLINE
int rugArgs() {
  using namespace arg;
           
  if(0) ;
  else if(argis("-starbattle")) {
    PHASEFROM(3);
    launch();
    }

  #if ISWEB
  else if(argis("-sb")) {
    PHASEFROM(3);
    launch();
    mapstream::loadMap("1");    
    mapeditor::drawplayer = false;
    }
  #endif

  else return 1;
  return 0;
  }

auto starbattle_hook = 
  addHook(hooks_args, 100, rugArgs) +
  addHook(hooks_drawcell, 100, draw_star) +
  addHook(mapstream::hooks_savemap, 100, [] (fhstream& f) {
    f.write<int>(isize(sdata));
    for(auto& sd: sdata) {
      f.write(mapstream::cellids[sd.first]);
      f.write(sd.second.region);
      f.write(sd.second.starred);
      f.write(sd.second.illegal);
      }
    f.write(stars_per_line);
    f.write(stars_per_region);
    }) +
  addHook(hooks_clearmemory, 40, [] () {
    sdata.clear();
    lines.clear();
    }) +
  addHook(mapstream::hooks_loadmap, 100, [] (fhstream& f) {
    int num;
    f.read(num);
    sdata.clear();
    for(int i=0; i<num; i++) {
      int32_t at = f.get<int>();
      cell *c = mapstream::cellbyid[at];
      auto& sd = sdata[c];
      f.read(sd.region);
      f.read(sd.starred);
      f.read(sd.illegal);
      }
    f.read(stars_per_line);
    f.read(stars_per_region);
    create_lines();
    });
#endif

EX }

EX }

