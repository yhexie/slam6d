/**
 * @file 
 * @brief Efficient representation of an octree
 * @author Jan Elsberg. Automation Group, Jacobs University Bremen gGmbH, Germany. 
 * @author Kai Lingemann. Institute of Computer Science, University of Osnabrueck, Germany.
 * @author Andreas Nuechter. Institute of Computer Science, University of Osnabrueck, Germany.
 */

#include <stdio.h>

#include <vector>
using std::vector;
#include <deque>
using std::deque;
#include <set>
using std::set;
#include <list>
using std::list;
#include <iostream>
#include <fstream>
#include <string>

#include "slam6d/globals.icc"
#include "slam6d/point_type.h"

#include "slam6d/Boctree.h"
#include "show/compacttree.h"
#include "show/colormanager.h"
#include "show/scancolormanager.h"
#include "show/viewcull.h"

compactTree::~compactTree(){
    deletetNodes(*root);
    delete root;

    delete[] mins;
    delete[] maxs;
  } 


  void compactTree::AllPoints( cbitoct &node, vector<double*> &vp, double center[3], double size) {
    double ccenter[3];
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          tshort *point = children->getPoints();
          lint length = children->getLength();

          for(unsigned int iterator = 0; iterator < length; iterator++ ) {
            double *p = new double[3];
            //cout << point[0] << " " << point[1] << " " << point[2] << endl; 
            for (unsigned int k = 0; k < 3; k++){
              p[k] = point[k] * precision + ccenter[k];
            }

            vp.push_back(p);
            point+=POINTDIM;
          }
        } else { // recurse
          AllPoints( children->node, vp, ccenter, size/2.0);
        }
        ++children; // next child
      }
    }
  }
  
  

  void compactTree::GetOctTreeCenter(vector<double*>&c, cbitoct &node, double *center, double size) {
    double ccenter[3];
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (unsigned char i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          double * cp = new double[POINTDIM];
          for (unsigned int iterator = 0; iterator < POINTDIM; iterator++) {
            cp[iterator] = ccenter[iterator];
          }
          c.push_back(cp);
        } else { // recurse
          GetOctTreeCenter(c, children->node, ccenter, size/2.0);
        }
        ++children; // next child
      }
    }
  }
/*
  void GetOctTreeRandom(vector<double*>&c, cbitoct &node) {
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf
          shortpointrep *points = children->points;
          int tmp = rand(points[0].length);
          tshort *point = &(points[POINTDIM*tmp+1].v);
          c.push_back(point);

        } else { // recurse
          GetOctTreeRandom(c, children->node);
        }
        ++children; // next child
      }
    }
  } 
  
  

  void GetOctTreeRandom(vector<double*>&c, unsigned int ptspervoxel, cbitoct &node) {
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf
          shortpointrep *points = children->points;
          unsigned int length = points[0].length;
          if (ptspervoxel >= length) {
            for (unsigned int j = 0; j < length; j++) 
              c.push_back(&(points[POINTDIM*j+1].v));

            ++children; // next child
            continue;
          }
          set<int> indices;
          while(indices.size() < ptspervoxel) {
            int tmp = rand(length-1);
            indices.insert(tmp);
          }
          for(set<int>::iterator it = indices.begin(); it != indices.end(); it++) 
            c.push_back(&(points[POINTDIM*(*it)+1].v));

        } else { // recurse
          GetOctTreeRandom(c, ptspervoxel, children->node);
        }
        ++children; // next child
      }
    }
  }
*/
  long compactTree::countNodes(cbitoct &node) {
    long result = 0;
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf
       //   ++result;
        } else { // recurse
          result += countNodes(children->node) + 1;
        }
        ++children; // next child
      }
    }
    return result;
  }

  long compactTree::countLeaves(cbitoct &node) {
    long result = 0;
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf
          lint nrpts = children->getLength();
          result += POINTDIM*nrpts + 1;
        } else { // recurse
          result += countLeaves(children->node);
        }
        ++children; // next child
      }
    }
    return result;
  }


  void compactTree::deletetNodes(cbitoct &node) {
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);
    bool haschildren = false;

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf
          tshort *points = children->getPoints();
          delete [] points;
//          delete [] children->points;
        } else { // recurse
          deletetNodes(children->node);
        }
        ++children; // next child
        haschildren = true;
      }
    }
    // delete children
    if (haschildren) {
      cbitoct::getChildren(node, children);
      delete[] children;
    }
  }

  
  unsigned long compactTree::maxTargetPoints( cbitoct &node ) {
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    unsigned long max = 0;

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          lint length = children->getLength();
          if (length > max) max = length;
        } else { // recurse
          unsigned long tp = maxTargetPoints( children->node);
          if (tp > max) max = tp;
        }
        ++children; // next child
      }
    }

    return max*POPCOUNT(node.valid);
  }
 
  void compactTree::displayOctTreeAll( cbitoct &node, double *center, double size) {
    double ccenter[3];
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          tshort *point = children->getPoints();
          lint length = children->getLength();
          glBegin(GL_POINTS);
          for(unsigned int iterator = 0; iterator < length; iterator++ ) {
            if(cm) cm->setColor(point);
            //cout << "C " << point[1] << " " << cm << endl;
            //glVertex3f( point[0], point[1], point[2]);
              glVertex3f( point[0] * precision + ccenter[0], point[1] * precision + ccenter[1], point[2] * precision + ccenter[2]);
            point+=POINTDIM;
          }
          glEnd();
        } else { // recurse
          displayOctTreeAll( children->node, ccenter, size/2.0);
        }
        ++children; // next child
      }
    }
  }

  void compactTree::displayOctTreeAllCulled( cbitoct &node, double *center, double size ) {
    int res = CubeInFrustum2(center[0], center[1], center[2], size);
    if (res==0) return;  // culled do not continue with this branch of the tree

    if (res == 2) { // if entirely within frustrum discontinue culling
      displayOctTreeAll(node, center, size);
      return;
    }

    double ccenter[3];
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          // check if leaf is visible
          if ( CubeInFrustum(ccenter[0], ccenter[1], ccenter[2], size/2.0) ) {
            tshort *point = children->getPoints();
            lint length = children->getLength();
            glBegin(GL_POINTS);
            for(unsigned int iterator = 0; iterator < length; iterator++ ) {
              if(cm) cm->setColor(point);
              //glVertex3f( point[0], point[1], point[2]);
              glVertex3f( point[0] * precision + ccenter[0], point[1] * precision + ccenter[1], point[2] * precision + ccenter[2]);
              point+=POINTDIM;
            }
            glEnd();
          }
        } else { // recurse
          displayOctTreeAllCulled( children->node, ccenter, size/2.0);
        }
        ++children; // next child
      }
    }
  }

  void compactTree::displayOctTreeCulledLOD(long targetpts, cbitoct &node, double *center, double size ) {
    if (targetpts <= 0) return; // no need to display anything

    int res = CubeInFrustum2(center[0], center[1], center[2], size);
    if (res==0) return;  // culled do not continue with this branch of the tree

    if (res == 2) { // if entirely within frustrum discontinue culling
      displayOctTreeLOD(targetpts, node, center, size);
      return;
    }

    double ccenter[3];
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    unsigned short nc = POPCOUNT(node.valid);
    long newtargetpts = targetpts;
    if (nc > 0) {
      newtargetpts = newtargetpts/nc;
      if (newtargetpts <= 0 ) return;
    }

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          // check if leaf is visible
          if ( CubeInFrustum(ccenter[0], ccenter[1], ccenter[2], size/2.0) ) {
            tshort *point = children->getPoints();
            lint length = children->getLength();
            glBegin(GL_POINTS);
            if (length > 10 && !LOD(ccenter[0], ccenter[1], ccenter[2], size/2.0) ) {  // only a single pixel on screen only paint one point
              if(cm) cm->setColor(point);
              //glVertex3f( point[0], point[1], point[2]);
              glVertex3f( point[0] * precision + ccenter[0], point[1] * precision + ccenter[1], point[2] * precision + ccenter[2]);
            } else if (length <= newtargetpts) {        // more points requested than possible, plot all
              for(unsigned int iterator = 0; iterator < length; iterator++ ) {
                if(cm) cm->setColor(point);
                //glVertex3f( point[0], point[1], point[2]);
              glVertex3f( point[0] * precision + ccenter[0], point[1] * precision + ccenter[1], point[2] * precision + ccenter[2]);
                point+=POINTDIM;
              }
            } else {                         // select points to show
              // TODO smarter subselection of points here
              double each = (double)POINTDIM * (double)((double)length/(double)newtargetpts);
              tshort *p;
              int index;
              for(unsigned int iterator = 0; iterator < newtargetpts; iterator++ ) {
                index = (double)iterator * each;
                p = point + index - index%POINTDIM;
                if(cm) cm->setColor(p);
                //glVertex3f( p[0], p[1], p[2]);
              glVertex3f( p[0] * precision + ccenter[0], p[1] * precision + ccenter[1], p[2] * precision + ccenter[2]);
                //point += each;
              }
            }
            glEnd();
          }

        } else { // recurse
          displayOctTreeCulledLOD(newtargetpts, children->node, ccenter, size/2.0);
        }
        ++children; // next child
      }
    }
  }

  void compactTree::displayOctTreeLOD(long targetpts, cbitoct &node, double *center, double size ) {
    if (targetpts <= 0) return; // no need to display anything

    double ccenter[3];
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    unsigned short nc = POPCOUNT(node.valid);
    long newtargetpts = targetpts;
    if (nc > 0) {
      newtargetpts = newtargetpts/nc;
      if (newtargetpts <= 0 ) return;
    }


    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          tshort *point = children->getPoints();
          lint length = children->getLength();
          glBegin(GL_POINTS);
/*          if (length > 10 && !LOD(ccenter[0], ccenter[1], ccenter[2], size/2.0) ) {  // only a single pixel on screen only paint one point
            if(cm) cm->setColor(point);
            //glVertex3f( point[0], point[1], point[2]);
              glVertex3f( point[0] * precision + ccenter[0], point[1] * precision + ccenter[1], point[2] * precision + ccenter[2]);
          } else*/ if (length <= newtargetpts) {        // more points requested than possible, plot all
            for(unsigned int iterator = 0; iterator < length; iterator++ ) {
              if(cm) cm->setColor(point);
              //glVertex3f( point[0], point[1], point[2]);
              glVertex3f( point[0] * precision + ccenter[0], point[1] * precision + ccenter[1], point[2] * precision + ccenter[2]);
              point+=POINTDIM;
            }
          } else {                         // select points to show
            // TODO smarter subselection of points here
            double each = (double)POINTDIM * (double)((double)length/(double)newtargetpts);
            tshort *p;
            int index;
            for(unsigned int iterator = 0; iterator < newtargetpts; iterator++ ) {
              index = (double)iterator * each;
              p = point + index - index%POINTDIM;
              if(cm) cm->setColor(p);
              //glVertex3f( p[0], p[1], p[2]);
              glVertex3f( p[0] * precision + ccenter[0], p[1] * precision + ccenter[1], p[2] * precision + ccenter[2]);
              //point += each;
            }
          }
          glEnd();
        } else { // recurse
          displayOctTreeLOD(newtargetpts, children->node, ccenter, size/2.0);
        }
        ++children; // next child
      }
    }
  }
  
  
  void compactTree::displayOctTreeCAllCulled( cbitoct &node, double *center, double size, double minsize ) {
    int res = CubeInFrustum2(center[0], center[1], center[2], size);
    if (res==0) return;  // culled do not continue with this branch of the tree

    if (res == 2) { // if entirely within frustrum discontinue culling
      displayOctTreeCAll(node, center, size, minsize);
      return;
    }

    double ccenter[3];
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf || minsize > size ) {   // if ith node is leaf get center
          // check if leaf is visible
          if ( CubeInFrustum(ccenter[0], ccenter[1], ccenter[2], size/2.0) ) {
            showCube(ccenter, size/2.0);
          }
        } else { // recurse
          displayOctTreeCAllCulled( children->node, ccenter, size/2.0, minsize);
        }
        ++children; // next child
      }
    }
  }
  
  void compactTree::displayOctTreeCAll( cbitoct &node, double *center, double size, double minsize ) {
    double ccenter[3];
    cbitunion<tshort> *children;
    cbitoct::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf || minsize > size ) {   // if ith node is leaf get center
          showCube(ccenter, size/2.0);
        } else { // recurse
          displayOctTreeCAll( children->node, ccenter, size/2.0, minsize);
        }
        ++children; // next child
      }
    }
  }

  void compactTree::showCube(double *center, double size) {
  glLineWidth(1.0);
    glBegin(GL_QUADS);      // draw a cube with 6 quads
glColor3f(0.0f,1.0f,0.0f);      // Set The Color To Green
    glVertex3f(center[0] + size, center[1] + size, center[2] - size);
    glVertex3f(center[0] - size, center[1] + size, center[2] - size);
    glVertex3f(center[0] - size, center[1] + size, center[2] + size);
    glVertex3f(center[0] + size, center[1] + size, center[2] + size);
  glColor3f(1.0f,0.5f,0.0f);      // Set The Color To Orange

    glVertex3f(center[0] + size, center[1] - size, center[2] + size); 
    glVertex3f(center[0] - size, center[1] - size, center[2] + size);
    glVertex3f(center[0] - size, center[1] - size, center[2] - size);
    glVertex3f(center[0] + size, center[1] - size, center[2] - size);

      glColor3f(1.0f,0.0f,0.0f);      // Set The Color To Red
    glVertex3f(center[0] + size, center[1] + size, center[2] + size); 
    glVertex3f(center[0] - size, center[1] + size, center[2] + size);
    glVertex3f(center[0] - size, center[1] - size, center[2] + size);
    glVertex3f(center[0] + size, center[1] - size, center[2] + size);

      glColor3f(1.0f,1.0f,0.0f);      // Set The Color To Yellow
    glVertex3f(center[0] + size, center[1] - size, center[2] - size); 
    glVertex3f(center[0] - size, center[1] - size, center[2] - size);
    glVertex3f(center[0] - size, center[1] + size, center[2] - size);
    glVertex3f(center[0] + size, center[1] + size, center[2] - size);

    glColor3f(0.0f,0.0f,1.0f);      // Set The Color To Blue
    glVertex3f(center[0] - size, center[1] + size, center[2] + size); 
    glVertex3f(center[0] - size, center[1] + size, center[2] - size);
    glVertex3f(center[0] - size, center[1] - size, center[2] - size);
    glVertex3f(center[0] - size, center[1] - size, center[2] + size);

    glColor3f(1.0f,0.0f,1.0f);      // Set The Color To Violet
    glVertex3f(center[0] + size, center[1] + size, center[2] - size); 
    glVertex3f(center[0] + size, center[1] + size, center[2] + size);
    glVertex3f(center[0] + size, center[1] - size, center[2] + size);
    glVertex3f(center[0] + size, center[1] - size, center[2] - size);


    glEnd();

  }
/*
  template <class P>
  compactTree::compactTree(P * const* pts, int n, double voxelSize, PointType<tshort> _pointtype , ScanColorManager<tshort> *scm ) : pointtype(_pointtype) {
    
    cm = 0;
    if (scm) {
      scm->registerTree(this);
      for (int i = 1; i < n; i++) {
        scm->updateRanges(pts[i]);
      }
    }
    this->voxelSize = voxelSize;

    this->POINTDIM = pointtype.getPointDim();

    mins = new double[POINTDIM];
    maxs = new double[POINTDIM];

    // initialising
    for (unsigned int i = 0; i < POINTDIM; i++) { 
      mins[i] = pts[0][i]; 
      maxs[i] = pts[0][i];
    }

    for (unsigned int i = 0; i < POINTDIM; i++) { 
      for (int j = 1; j < n; j++) {
        mins[i] = min(mins[i], (double)pts[j][i]);
        maxs[i] = max(maxs[i], (double)pts[j][i]);
      }
    }

    center[0] = 0.5 * (mins[0] + maxs[0]);
    center[1] = 0.5 * (mins[1] + maxs[1]);
    center[2] = 0.5 * (mins[2] + maxs[2]);
    size = max(max(0.5 * (maxs[0] - mins[0]), 0.5 * (maxs[1] - mins[1])), 0.5 * (maxs[2] - mins[2]));

    size += 1.0; // some buffer for numerical problems

    double vs = size;
    while (vs > voxelSize) {
      vs = vs/2.0;
    }
//    vs = vs/2.0;
//    double precision = vs/ pow(2, sizeof(tshort)*8-1);
    precision = vs/ pow(2, 15);
    // vs is the real voxelsize
    cout << "real voxelsize is " << vs << endl;
    cout << "precision is now " << precision << endl;

    // calculate new buckets
    double newcenter[8][3];
    double sizeNew = size / 2.0;

    for (unsigned char i = 0; i < 8; i++) {
      childcenter(center, newcenter[i], size, i);
    }
    // set up values
    root = new cbitoct();

    countPointsAndQueue(pts, n, newcenter, sizeNew, *root);
  }
*/


  
  

template <class T>
  void compactTree::selectRay(vector<T *> &points) { 
    //selectRay(points, *root, center, size); 
  }
  
  
void compactTree::childcenter(double *pcenter, double *ccenter, double size, unsigned char i) {
    switch (i) {
      case 0:
        ccenter[0] = pcenter[0] - size / 2.0;
        ccenter[1] = pcenter[1] - size / 2.0;
        ccenter[2] = pcenter[2] - size / 2.0;
        break;
      case 1:
        ccenter[0] = pcenter[0] + size / 2.0;
        ccenter[1] = pcenter[1] - size / 2.0;
        ccenter[2] = pcenter[2] - size / 2.0;
        break;
      case 2:
        ccenter[0] = pcenter[0] - size / 2.0;
        ccenter[1] = pcenter[1] + size / 2.0;
        ccenter[2] = pcenter[2] - size / 2.0;
        break;
      case 3:
        ccenter[0] = pcenter[0] - size / 2.0;
        ccenter[1] = pcenter[1] - size / 2.0;
        ccenter[2] = pcenter[2] + size / 2.0;
        break;
      case 4:
        ccenter[0] = pcenter[0] + size / 2.0;
        ccenter[1] = pcenter[1] + size / 2.0;
        ccenter[2] = pcenter[2] - size / 2.0;
        break;
      case 5:
        ccenter[0] = pcenter[0] + size / 2.0;
        ccenter[1] = pcenter[1] - size / 2.0;
        ccenter[2] = pcenter[2] + size / 2.0;
        break;
      case 6:
        ccenter[0] = pcenter[0] - size / 2.0;
        ccenter[1] = pcenter[1] + size / 2.0;
        ccenter[2] = pcenter[2] + size / 2.0;
        break;
      case 7:
        ccenter[0] = pcenter[0] + size / 2.0;
        ccenter[1] = pcenter[1] + size / 2.0;
        ccenter[2] = pcenter[2] + size / 2.0;
        break;
      default:
        break;
    }
  }
  

void compactTree::GetOctTreeCenter(vector<double*>&c) { GetOctTreeCenter(c, *root, center, size); }
/*  void GetOctTreeRandom(vector<double*>&c) { GetOctTreeRandom(c, *root); }
  void GetOctTreeRandom(vector<double*>&c, unsigned int ptspervoxel) { GetOctTreeRandom(c, ptspervoxel, *root); }*/
  void compactTree::AllPoints(vector<double *> &vp) { AllPoints(*compactTree::root, vp, center, size); }

  long compactTree::countNodes() { return 1 + countNodes(*root); }
  long compactTree::countLeaves() { return 1 + countLeaves(*root); }

  void compactTree::setColorManager(ColorManager *_cm) { cm = _cm; }

  void compactTree::displayOctTreeCulled(long targetpts) { 
    displayOctTreeCulledLOD(targetpts, *root, center, size); 
  }
  
  void compactTree::displayOctTreeAllCulled() { 
    //displayOctTreeAllCulled(*root, center, size); 
    displayOctTreeAll(*root, center, size); 
  }
  
  void compactTree::displayOctTree(double minsize ) { 
    displayOctTreeCAllCulled(*root, center, size, minsize); 
  }
  unsigned long compactTree::maxTargetPoints() {
    return maxTargetPoints(*root);
  }

  shortpointrep* compactTree::createPoints(lint length) {
    shortpointrep *points = new shortpointrep[POINTDIM*length];
    return points;
  }