// Hyperbolic Rogue -- the Tutorial/presentation
// Copyright (C) 2011-2017 Zeno Rogue, see 'hyper.cpp' for details

namespace tour {

bool on;

bool texts = true;

string tourhelp;

int currentslide;

const int LEGAL_NONE=0;
const int LEGAL_UNLIMITED=1;
const int LEGAL_HYPERBOLIC=2;
const int LEGAL_ANY=3;
const int LEGAL_NONEUC=4;
const int QUICKSKIP=8;
const int FINALSLIDE=16;

// #ifdef PRES
// #include "presentation.cpp"
// #else

struct slide { 
  const char *name; int unused_id; int flags; const char *help; 
  function<void(presmode mode)> action;
  } ;

extern slide slides[];

// modes: 
//   1 - enter the slide
//   2 - each frame
//   3 - leave the slide
//   4 - quicken or modify the slide
//   5 - on initgame

void setCanvas(presmode mode, char canv) {
  static char wc;
  static eLand ld;
  if(mode == pmStart) {
    wc = mapeditor::whichCanvas;
    mapeditor::whichCanvas = canv;
    ld = firstland;
    firstland = laCanvas;
    restartGame(0, true);
    }
  if(mode == pmStop) {
    mapeditor::whichCanvas = wc;
    firstland = ld;
    restartGame(0, false);
    }
  }

bool sickmode;

function<eLand(eLand)> getNext;
function<bool(eLand)> quickfind;
function<bool(eLand)> showland;

#define GETNEXT getNext = [](eLand old)
#define QUICKFIND quickfind = [](eLand l)
#define SHOWLAND(f) showland = [](eLand l) { return f; }

string slidecommand;

void presentation(presmode mode) {

#ifdef PRES
  presentation2(mode);
#endif

  cheater = 0;
  
  if(mode == pmStart) tourhelp = slides[currentslide].name;
  
  if(sickmode && !items[itOrbTeleport]) items[itOrbTeleport] = 1;
  if(mode == pmStart) slidecommand = "";

  GETNEXT { return laNone; };
  QUICKFIND { return false; };
  SHOWLAND(true);
  
  slides[currentslide].action(mode);
  }

void slidehelp() {
  if(texts) {
    help = 
      helptitle(slides[currentslide].name, 0xFF8000) + 
      slides[currentslide].help;
    if(cmode != emHelp)  
      lastmode = cmode;
    cmode = emHelp; 
    }
  }

bool handleKeyTour(int sym, int uni) {
  bool normode = (cmode == emHelp || cmode == emNormal || cmode == emQuit);
  if(!normode) return false;
  int flags = slides[currentslide].flags;
  if((sym == SDLK_RETURN || sym == SDLK_KP_ENTER) && (cmode != emHelp || (flags & QUICKSKIP))) {
    if(geometry || purehepta) { restartGame(0, false); return true; }
    if(flags & FINALSLIDE) return true;
    presentation(pmStop);
    currentslide++;
    slidehelp();
    presentation(pmStart);
    return true;
    }
  if(sym == SDLK_BACKSPACE) {
    if(geometry || purehepta) { restartGame(0, false); return true; }
    if(currentslide == 0) { slidehelp(); return true; }
    presentation(pmStop);
    currentslide--;
    presentation(pmStart);
    if(cmode == emHelp) slidehelp();
    return true;
    }
  if(sym == '1' || sym == '2' || sym == '3') {
    int legal = slides[currentslide].flags & 7;

    if(legal == LEGAL_NONE) {
      addMessage(XLAT("You cannot change geometry in this slide."));
      return true;
      }
    if(legal == LEGAL_UNLIMITED && sym == '1') {
      addMessage(XLAT("This does not work in bounded geometries."));
      return true;
      }
    if(legal == LEGAL_NONEUC && sym == '2') {
      addMessage(XLAT("This does not work in Euclidean geometry."));
      return true;
      }
    if(legal == LEGAL_HYPERBOLIC && sym != '3') {
      addMessage(XLAT("This works only in hyperbolic geometry."));
      return true;
      }
    
    if(sym == '2') {
      bool ok = cwt.c->land == laCanvas;
      for(int i=0; i<LAND_EUC; i++) if(land_euc[i] == cwt.c->land) ok = true;
      if(!ok) {
        addMessage(XLAT("This land has no Euclidean version."));
        return true;
        }
      }
    
    if(sym == '1') {
      bool ok = cwt.c->land == laCanvas;
      for(int i=0; i<LAND_SPH; i++) if(land_sph[i] == cwt.c->land) ok = true;
      if(!ok) {
        addMessage(XLAT("This land has no spherical version."));
        return true;
        }
      }
    
    if(geometry || purehepta) {
      restartGame(0, false); 
      presentation(pmGeometryReset);
      return true;
      }

    presentation(pmGeometry);

    if(sym == '1') targetgeometry = gSphere;
    if(sym == '2') targetgeometry = gEuclid;
    firstland = euclidland = cwt.c->land;
    restartGame(sym == '3' ? '7' : 'g', true);
    presentation(pmGeometryStart);
    return true;
    }
  if(sym == '4') {
    cmode = emNormal;
    if(items[itOrbTeleport]) goto give_aether;
    items[itOrbTeleport] = 1;
    checkmove();
    if(!canmove) {
      if(items[itOrbAether]) goto give_flash;
      give_aether:
      items[itOrbAether] = 10;
      checkmove();
      if(!canmove) {
        give_flash:
        activateFlash();
        canmove = true;
        }
      }
    return true;
    }
  if(sym == '5') {
    presentation(pmKey);
    return true;
    }
  if(sym == '6') {
    sickmode = !sickmode;
    static ld spd;
    if(sickmode == true) {
      spd = vid.sspeed, vid.sspeed = 5;
      addMessage("Static mode enabled.");
      }
    else {
      vid.sspeed = spd;
      addMessage("Static mode disabled.");
      }
    return true;
    }
  if(sym == '7') {
    texts = !texts;
    if(texts) slidehelp();
    else addMessage("Help texts disabled.");
    return true;
    }
  if(sym == '8') {
    conformal::includeHistory = !conformal::includeHistory;
    return true;
    }
  return false;
  }

void checkGoodLand(eLand l) {
  if(!showland(l) && texts) {
    help = XLAT(
      "This tutorial is different than most other game tutorials -- "
      "you are not forced to do anything, and you can go wherever you want.\n\n"
      "However, %the1 is not what we are talking about now. "
      "We will not explain this land at the moment, and you could ponentially "
      "get lost there.\n\n"
      "Remember that you can get to the next slide by pressing Enter.",
      l
      );
    cmode = emHelp;
    }
  }

void start() {
  currentslide = 0;
  vid.scale = 1;
  vid.alpha = 1;
  pmodel = mdDisk;
  presentation(pmStartAll);
  restartGame('T');
  if(tour::on) {
    presentation(pmStart);
    slidehelp();
    }
  }

#ifdef OTHERSLIDES
#include "other-slides.cpp"
#else

slide slides[] = {
  {"Introduction", 10, LEGAL_ANY | QUICKSKIP,
    "This tutorial is mostly aimed to show what is "
    "special about the geometry used by HyperRogue. "
    "It also shows the basics of gameplay, and "
    "how is it affected by geometry.\n\n"
    "You decide when you want to stop playing with the "
    "current \"slide\" and go to the next one, by pressing Enter. You can also "
    "press ESC to see a "
    "menu with other options.",
    [] (presmode mode) {
      if(mode == 1) {
        if(tour::texts) addMessage(XLAT("Welcome to the HyperRogue tutorial!"));
        else clearMessages();  
        }  
      SHOWLAND( l == laIce );
      }
    },
  {"Basics of gameplay", 11, LEGAL_ANY,
    "The game starts in the Icy Lands. Collect the Ice Diamonds "
    "(press F1 if you do not know how to move). "
    "After you collect many of them, monsters will start to pose a challenge.\n"
    "As is typical in roguelikes and other games based on tactical skill rather "
    "than story, if you lose, you have to start a new one from the start. "
    "However, in this tutorial, you can simply press '4' "
    "to teleport away from a bad situation."
    "In general, the tutorial is rigged to show you what it "
    "wants -- for example, in this slide, you can press '5' to get "
    "lots of Ice Diamonds quickly.",
    [] (presmode mode) {
      slidecommand = "gain Ice Diamonds";
      if(mode == 4)
        forCellEx(c2, cwt.c) 
        forCellEx(c3, c2)
        if(c3->wall == waNone && c3->item == itNone && c3->monst == moNone && c3 != cwt.c) 
          c3->item = itDiamond;
      SHOWLAND( l == laIce );
      }
    }, 
  {"Hypersian Rug model", 21, LEGAL_HYPERBOLIC,
    "New players think that the action of HyperRogue takes place on a sphere. "
#ifdef WEB
    "This is not true -- try the Tutorial in the native desktop version shows "
    "the surface HyperRogue actually takes place on.",
#else
    "This is not true -- the next slide will show the surface HyperRogue "
    "actually takes place on.\n\n"
    "Use arrow keys to rotate the model, and Page Up/Down to zoom.\n\n"
    "If you do not see anything, press '5' to try a safer renderer.",
#endif
    [] (presmode mode) {
      static int wm, mm;
#ifndef NORUG
      if(mode == 1) {
        rug::init();
        wm = vid.wallmode;
        mm = vid.monmode;
        vid.wallmode = 3;
        vid.monmode = 2;
        }
      if(mode == 3) {
        rug::close();
        vid.wallmode = wm;
        vid.monmode = mm;
        }
      slidecommand = "switch renderer";
      if(mode == 4) {
        rug::close();
        rug::rendernogl = !rug::rendernogl;
        rug::init();
        }
#endif
      SHOWLAND( l == laIce );
      }
    }, 
  {"Expansion", 22, LEGAL_ANY,
    "The next slide shows the number of cells in distance 1, 2, 3, ... from you. "
    "It grows exponentially: there are more than 10^100 cells "
    "in radius 1000 around you, and you will move further away during the game!\n\n"
    "This is extremely important in the design of HyperRogue. "
    "HyperRogue has many navigational puzzles -- what would be simple in Euclidean world "
    "is extremely tricky "
    "in hyperbolic geometry (you want to reach a specific location 20 cells away, "
    "which of the thousands of possible directions should you take?); however, other things virtually impossible in Euclidean "
    "world become easy in HyperRogue. "
    "HyperRogue had to be specially designed so that it is impossible to grind the "
    "infinite world. There are almost no permanent upgrades; collecting treasures "
    "brings you benefits, but trying to get too many of the same kind is extremely dangerous.",
    [] (presmode mode) {
      if(mode == 1) viewdists = true;
      if(mode == 2) viewdists = canmove; // disable when killed
      if(mode == 3) viewdists = false;
      SHOWLAND( l == laIce );
      }
    },
  {"Tiling and Tactics", 23, LEGAL_ANY, 
    "The tactics of fighting simple monsters, such as the Yetis from the Icy Lands, "
    "might appear shallow, but hyperbolic geometry is essential even there. "
    "In the next slide, you are attacked by two monsters at once. "
    "You can make them line up simply by "
    "running away in a straight line. "
    "Press '2' to try the same in the Euclidean world -- it is impossible.",
    [] (presmode mode) {
      setCanvas(mode, 'F');
      if(mode == 5) {
        cwt.c->mov[0]->monst = moRunDog;
        cwt.c->mov[1]->monst = moGoblin;
        }
      SHOWLAND( l == laCanvas );
      }
    },
  {"Straight Lines", 24, LEGAL_ANY, 
    "Hyperbolic geometry has been discovered by the 19th century mathematicians who "
    "wondered about the nature of paralellness. Take a line L and a point A. "
    "Can a world exist where there is more than one line passing through A "
    "which does not cross L?\n\n"
    "The Icy Land will be very dangerous if you have lots of Ice Diamonds -- "
    "lots of Yetis and Ice Wolves hunting you! But the other lands, where "
    "you have no treasures yet, will still be (relatively) safe.\n\n"
    "Wander further, and you should find Crossroads quickly -- "
    "the Great Walls are straight lines, and indeed, they work differently than in "
    "Euclidean. On the other side of Great Walls, you see other lands -- "
    "there are about 50 lands in HyperRogue, based "
    "on different mechanics and aspects of hyperbolic geometry.",
    [] (presmode mode) {
      GETNEXT {
        if(isCrossroads(old)) 
          return pick(
            pick(laRedRock, laWarpCoast, laMirror),
            pick(laLivefjord, laAlchemist, laHell),
            pick(laJungle, laDesert, laRose),
            pick(laGraveyard, laMotion, laDryForest)
            );
        else return laCrossroads;
        };
      SHOWLAND( l == laCrossroads || l == laIce );
      }
    },
  {"Running Dogs", 25, LEGAL_ANY,
    "To learn more about straight lines, "
    "wander further, and you should find the Land of Eternal Motion. "
    "Try to run in a straight line, with a Running Dog next to you. "
    "Even though the Running Dog runs at the same speed as you, "
    "it will appear to go slower -- this is because you are running "
    "in a straight line, and the Running Dog has to run in a curve "
    "called an equidistant.\n\n"
#ifdef MAC
    "Remember that you can click with right Shift on anything to get more information.",
#else
    "Remember that you can right click on anything to get more information.",
#endif
    [] (presmode mode) {
      GETNEXT {
        if(isCrossroads(old)) return pick(laMotion, laNone);
        else if(old == laMotion) return laCrossroads;
        else return laMotion;
        };
      SHOWLAND( l == laCrossroads || l == laMotion );
      }
    },
  {"Equidistants", 27, LEGAL_ANY,
    "Equidistants are curves which are at some fixed distance from a "
    "straight line. Some lands in HyperRogue are based on equidistants; "
    "you should see them after wandering a bit more.\n\n"
    "This tutorial gives you freedom to go wherever you choose, "
    "but we do not recommend going deep into the Dungeon or the Ocean -- "
    "getting back might be difficult.",
    [] (presmode mode) {
      GETNEXT {
        if(isCrossroads(old))
          return hrand(100) < 20 ? laNone : 
            pick(laOcean, laIvoryTower, laDungeon, laEndorian);
        else return laCrossroads;
        };
      SHOWLAND( l == laCrossroads || l == laDungeon || l == laOcean || l == laIvoryTower || l == laEndorian );
      }
    },
  {"Circles", 26, LEGAL_ANY,
    "Circles are strange in hyperbolic geometry too. "
    "Look for the Castle of Camelot in the Crossroads; "
    "the Round Table inside is a circle of radius 28. "
    "Finding its center is a difficult challenge.\n\n"
    "Press '5' to cheat by seeing the smaller circles too.\n\n"
    "Note: Camelot and some other lands are unlocked earlier in the Tutorial than in a real game.",
    [] (presmode mode) {
      slidecommand = "enable the Camelot cheat";
      if(mode == 4) camelotcheat = !camelotcheat;
      GETNEXT {
        if(!isCrossroads(old)) return laCrossroads;
        return laNone;
        };
      QUICKFIND {
        return (l == laCamelot);
        };
      SHOWLAND( l == laCrossroads || l == laCamelot );
      }
    },
  {"Horocycles", 28, LEGAL_ANY,
    "Horocycles are similar to circles, but you cannot reach their center at all -- "
    "they can be understood as limit circles of infinite radius centered in some point "
    "in infinity (also called an ideal point).\n\n"
    "Go to R'Lyeh, and you should quickly find a Temple of Cthulhu there. "
    "Each circle of columns is actually a horocycle. Horocycles in a given "
    "temple are concentric, and there is an infinite number of them.",
    [] (presmode mode) {
      GETNEXT {
        if(isCrossroads(old))
          return pick(laRlyeh, laNone, laNone);
        else return pick(laCrossroads, old == laRlyeh ? laNone : laRlyeh);
        };
      QUICKFIND {
        return (l == laTemple);
        };
      SHOWLAND ( l == laCrossroads || l == laRlyeh || l == laTemple );
      }
    },
  {"Half-plane model", 47, LEGAL_HYPERBOLIC,
    "The game is normally displayed in the so called Poincar� disk model, "
    "which is a kind of a map of the infinite hyperbolic world. "
    "There are many projections of Earth, but since Earth is curved, "
    "all of them have to distort distances or angles in some way -- "
    "the same is true in hyperbolic geometry. "
    "The next slide shows another model, called the Poincar� upper half-plane model. In this model, "
    "horocycles centered at one specific ideal point are drawn as straight lines.",
    [] (presmode mode) {
      if(mode == 1) 
        pmodel = mdHalfplane;
      if(mode == 2) 
        conformal::rotation = cwt.c->land == laDungeon ? 0 : 2;
      if(mode == 3) pmodel = mdDisk, conformal::rotation = 0;
      }
    },
  {"Curvature", 29, LEGAL_ANY,
    "Now, go to the Burial Grounds and find an Orb of the Sword. The Sword appears to "
    "always be facing in the same direction whatever you do, and it appears that "
    "you have to rotate the sword to excavate the treasures; "
    "yet, it is possible to excavate them! You migth have already noticed "
    "that the world rotates after you move around a loop and return to an old "
    "place.\n\n"
    "This is related to the fact that the world of HyperRogue is curved, and "
    "the sum of angles in a triangle is not equal to 180 degrees.",
    [] (presmode mode) {
      slidecommand = "gain Orb of the Sword";
      if(mode == 4)
        items[itOrbSword] = 90;
      GETNEXT {
        if(isCrossroads(old))
          return pick(laBurial, laNone, laNone);
        else return pick(laCrossroads, old == laBurial ? laNone : laBurial);
        };
      QUICKFIND {
        return (l == laBurial && !items[itOrbSword]);
        };
      SHOWLAND ( l == laCrossroads || l == laBurial );
      }
    },
  {"Periodic patterns", 30, LEGAL_UNLIMITED,
    "Hyperbolic geometry yields much more interesting periodic patterns "
    "than Euclidean.",
    [] (presmode mode) {
      setCanvas(mode, 't');
      if(mode == 1) 
        mapeditor::displaycodes = 2,
        mapeditor::whichPattern = 'z';
      if(mode == 3) 
        mapeditor::displaycodes = 0,
        mapeditor::whichPattern = 0;
      SHOWLAND ( l == laCanvas );
      }
    },
  {"Periodic patterns: application", 31, LEGAL_ANY,
    "Many lands in HyperRogue are based around periodic patterns. "
    "For example, both Zebra and Windy Plains are based on the pattern "
    "shown in the previous slide. "
    "Such lands often have tree-like nature.",
    [] (presmode mode) {
      GETNEXT {
        if(isCrossroads(old))
          return pick(
            pick(laWineyard, laEmerald, laPower),
            pick(laZebra, laWhirlwind),
            laPalace, laNone
            );
        else return laCrossroads;
        };
      SHOWLAND ( l == laCrossroads ||
      l == laZebra || l == laWhirlwind || l == laPalace || l == laPrairie ||
      l == laEmerald || l == laWineyard || l == laPower );
      }
    },
  {"Fractal landscapes", 32, LEGAL_UNLIMITED,
    "On the following slide, the colors change smoothly in the whole infinite world. "
    "Again, this works better than in Euclidean geometry.",
    [] (presmode mode) {
      setCanvas(mode, 'l');
      SHOWLAND ( l == laCanvas );
      }
    },
  {"Fractal landscapes: application", 33, LEGAL_ANY,
    "This is applied in HyperRogue to create landscapes, such as the chasms in the "
    "land of Reptiles or the Dragon Chasms, which you should find quickly. "
    "Also in the Dragon Chasms, you can find a Baby Tortoise, and try to find "
    "a matching adult tortoise in the Gal�pagos. "
    "There are over two millions of species, but since there is so much space in "
    "hyperbolic geometry, finding a matching tortoise is possible. The brighter "
    "the color in Gal�pagos is, the more aspects of the tortoises in the given "
    "area are matching.",
    [] (presmode mode) {
      slidecommand = "create a baby tortoise";
      if(mode == 4) {
        cell *c = cwt.c->mov[0];
        c->item = itBabyTortoise;
        tortoise::babymap[c] = getBits(c) ^ tortoise::getRandomBits();
        }
      GETNEXT {
        if(old == laDragon) return pick(laTortoise, laTortoise, laCrossroads);
        else if(isCrossroads(old))
          return pick(laDragon, laReptile, laNone);
        return laNone;
        };
      QUICKFIND {
        return l == laTortoise && !items[itBabyTortoise];
        };
      SHOWLAND ( l == laCrossroads || l == laReptile || l == laDragon || l == laTortoise );
      }
    },
  {"Poincar� Ball model", 41, LEGAL_HYPERBOLIC,
    "The Poincar� disk model is a model of a hyperbolic *plane* -- you "
    "might wonder why are the walls rendered in 3D then.\n\n"
    "HyperRogue actually assumes that the floor level is an equidistant surface "
    "in a three-dimensional hyperbolic world, and the camera is placed above the "
    "plane that the surface is equidistant to (which boils down to showing "
    "the floor level in Poincar� disk model).\n\n"
    "This is shown on the next slide, in the Poincar� ball model, which is "
    "the 3D analog of the Poincar� disk model.",
    [] (presmode mode) {
      if(mode == 1) pmodel = mdBall;
      if(mode == 3) pmodel = mdDisk;
      }
    },
  {"Hyperboloid model", 42, LEGAL_ANY,
    "Let's see more models of the hyperbolic plane. "
    "This model uses a hyperboloid in the Minkowski geometry; "
    "it is used internally by HyperRogue.",
    [] (presmode mode) {
      if(mode == 1) pmodel = mdHyperboloid;
      if(mode == 3) pmodel = mdDisk;
      }
    },
  {"Beltrami-Klein model", 43, LEGAL_ANY,
    "This model renders straight lines as straight, but it distorts angles.",
    [] (presmode mode) {
      if(mode == 1) vid.alpha = 0;
      if(mode == 3) vid.alpha = 1;
      }
    },
  {"Gans model", 44, LEGAL_ANY,
    "Yet another model, which corresponds to orthographic projection of the "
    "sphere. Poincar� disk model, Beltrami-Klein model, and the Gans "
    "model are all obtained by looking at either the hyperboloid model or an "
    "equidistant surface from various distances.",
    [] (presmode mode) {
      if(mode == 1) vid.alpha = 400, vid.scale = 150;
      if(mode == 3) vid.alpha = vid.scale = 1;
      }
    },
  {"Band model", 45, LEGAL_NONEUC, 
    "The band model is the hyperbolic analog of the Mercator projection of the sphere: "
    "a given straight line is rendered as a straight line, and the rest of the "
    "world is mapped conformally, that is, angles are not distorted. "
    "Here, we take the straight line connecting your starting point and your "
    "current position -- usually the path taken by the player is surprisingly "
    "close to a straight line. Press '8' to see this path.\n\n"
    "If you want, press '5' to see it rendered as a spiral, although it takes lots of time and "
    "memory.",
    [] (presmode mode) {
      if(mode == 1) pmodel = mdBand, conformal::create(), conformal::rotation = 0;
      if(mode == 3) {
        conformal::clear(), pmodel = mdDisk;
        resetview();
        drawthemap();
        centerpc(INF);
        conformal::includeHistory = false;
        }
      slidecommand = "render spiral";
      if(mode == 4) conformal::createImage(true);
      if(mode == 11) conformal::create();
      if(mode == 13) conformal::clear();
      }
    },
  {"Conformal square model", 46, LEGAL_HYPERBOLIC,
    "The world can be mapped conformally to a square too.",
    [] (presmode mode) {
      if(mode == 1) pmodel = mdPolygonal, polygonal::solve();
      if(mode == 3) pmodel = mdDisk;
      }
    },
#ifdef ROGUEVIZ
  {"Collatz conjecture", 51, LEGAL_NONE,
    "Your version of HyperRogue includes RogueViz, which "
    "is an adaptation of HyperRogue as a visualization tool "
    "rather than a game. Hyperbolic space is great "
    "for visualizing some kinds of data because of the vast amount "
    "of space.\n\n"
    "The following slide is a visualization of the Collatz conjecture. "
    "Press '5' for a spiral rendering of the Collatz conjecture visualization.",
    [] (presmode mode) {
      setCanvas(mode, 'd');
      if(mode == 1) {
        rogueviz::dftcolor = 0x206020FF;
  
        rogueviz::collatz::s2 = .3;
        rogueviz::collatz::p2 = .5;
        rogueviz::collatz::s3 = -.4;
        rogueviz::collatz::p3 = .4;
  
        rogueviz::showlabels = true;
        
        rogueviz::on = true;
        gmatrix.clear();
        drawthemap();
        gmatrix0 = gmatrix;
  
        rogueviz::collatz::start();
        }
      
      if(mode == 3) {
        rogueviz::close();
        shmup::clearMonsters();
        }
  
      slidecommand = "render spiral";
      if(mode == 4) {
        pmodel = mdBand, conformal::create(), conformal::rotation = 0,
        conformal::createImage(true),
        conformal::clear(), pmodel = mdDisk;      
        }
      }
    },
#endif
#ifndef WEB
  {"Shoot'em up mode", 52, LEGAL_NONE,
    "In the shoot'em up mode, space and time is continuous. "
    "You attack by throwing knives. "
    "Very fun with two players!\n\n"
    "There are other special modes too which change the gameplay or "
    "focus on a particular challenge.",
    [] (presmode mode) {
      if(mode == 1) {
        firstland = cwt.c->land;
        restartGame('s', true);
        }
      if(mode == 3) {
        shmup::clearMonsters();
        restartGame(0, false);
        }    
      }
    },
#endif
  {"THE END", 99, LEGAL_ANY | FINALSLIDE,
    "This tour shows just a small part of what you can see in the world of HyperRogue. "
    "For example, "
    "hyperbolic mazes are much nicer than their Euclidean counterparts. "
    "Have fun exploring!\n\n"
    "Press '5' to leave the tutorial mode.",
    [] (presmode mode) {
      slidecommand = "leave the Tutorial";
      if(mode == 4) restartGame('T');
      }
    }
  };
#endif

}
