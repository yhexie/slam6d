/**
 * @file 
 * @brief Representation of an octree for show
 * @author Jan Elseberg. Jacobs University Bremen gGmbH, Germany
 * @author Andreas Nuechter. Jacobs University Bremen gGmbH, Germany
 */

#ifndef SHOWBOCTREE_H
#define SHOWBOCTREE_H


#include "../Boctree.h"
#include "colormanager.h"
#include "scancolormanager.h"
#include "viewcull.h"

template <class T> class ScanColorManager;

/**
 * @brief Octree for show
 * 
 * A cubic bounding box is calculated
 * from the given 3D points. Then it
 * is recusivly subdivided into smaller
 * subboxes
 *
 * It contains software culling functionalities
 */
template <class T> class Show_BOctTree : public BOctTree<T>  {

public:

  Show_BOctTree(T **pts, int n, T voxelSize, unsigned int pointdim = 3, ScanColorManager<T> *scm = 0)
    : BOctTree<T>(pts, n, voxelSize, pointdim) {
    cm = 0;
    if (scm) {
      scm->registerTree(this);
      for (int i = 1; i < n; i++) {
        scm->updateRanges(pts[i]);
      }
    }
  }

  Show_BOctTree(std::string filename, ScanColorManager<T> *scm = 0) : BOctTree<T>(filename)
  {
    if (scm) {
      scm->registerTree(this);
    }
    cm = 0;
  }

  void setColorManager(ColorManager<T> *_cm) { cm = _cm; }

  void displayOctTreeCulled(long targetpts) { 
    displayOctTreeCulledLOD(targetpts, *BOctTree<T>::root, BOctTree<T>::center, BOctTree<T>::size); 
  }
  
  void displayOctTreeAllCulled() { 
    displayOctTreeAll(*BOctTree<T>::root, BOctTree<T>::center, BOctTree<T>::size); 
  }

  void selectRay(vector<T *> &points) { 
    selectRay(points, *BOctTree<T>::root, BOctTree<T>::center, BOctTree<T>::size); 
  }
  void selectRay(T * &point) { 
    selectRay(point, *BOctTree<T>::root, BOctTree<T>::center, BOctTree<T>::size, FLT_MAX); 
  }

protected:
  
  void displayOctTreeAll( bitoct<T> &node, T *center, T size ) {
    T ccenter[3];
    bitunion<T> *children;
    bitoct<T>::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          pointrep *points = children->points;
          unsigned int length = points[0].length;
          T *point = &(points[1].v);  // first point
          glBegin(GL_POINTS);
          for(unsigned int iterator = 0; iterator < length; iterator++ ) {
            if(cm) cm->setColor(point);
            glVertex3f( point[0], point[1], point[2]);
            point+=BOctTree<T>::POINTDIM;
          }
          glEnd();
        } else { // recurse
          childcenter(center, ccenter, size, i);  // childrens center
          displayOctTreeAll( children->node, ccenter, size/2.0);
        }
        ++children; // next child
      }
    }
  }

  void displayOctTreeAllCulled( bitoct<T> &node, T *center, T size ) {
    int res = CubeInFrustum2(center[0], center[1], center[2], size);
    if (res==0) return;  // culled do not continue with this branch of the tree

    if (res == 2) { // if entirely within frustrum discontinue culling
      displayOctTreeAllCulled(node, center, size);
      return;
    }

    T ccenter[3];
    bitunion<T> *children;
    bitoct<T>::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          // check if leaf is visible
          if ( CubeInFrustum(ccenter[0], ccenter[1], ccenter[2], size/2.0) ) {
            pointrep *points = children->points;
            unsigned int length = points[0].length;
            T *point = &(points[1].v);  // first point
            glBegin(GL_POINTS);
            for(unsigned int iterator = 0; iterator < length; iterator++ ) {
              if(cm) cm->setColor(point);
              glVertex3f( point[0], point[1], point[2]);
              point+=BOctTree<T>::POINTDIM;
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

  void displayOctTreeCulledLOD(long targetpts, bitoct<T> &node, T *center, T size ) {
    if (targetpts <= 0) return; // no need to display anything

    int res = CubeInFrustum2(center[0], center[1], center[2], size);
    if (res==0) return;  // culled do not continue with this branch of the tree

    if (res == 2) { // if entirely within frustrum discontinue culling
      displayOctTreeLOD(targetpts, node, center, size);
      return;
    }

    T ccenter[3];
    bitunion<T> *children;
    bitoct<T>::getChildren(node, children);

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
            pointrep *points = children->points;
            unsigned int length = points[0].length;
            T *point = &(points[1].v);  // first point
            glBegin(GL_POINTS);

            if (length > 10 && !LOD(ccenter[0], ccenter[1], ccenter[2], size/2.0) ) {  // only a single pixel on screen only paint one point
              if(cm) cm->setColor(point);
              glVertex3f( point[0], point[1], point[2]);
            } else if (length <= newtargetpts) {        // more points requested than possible, plot all
              for(unsigned int iterator = 0; iterator < length; iterator++ ) {
                if(cm) cm->setColor(point);
                glVertex3f( point[0], point[1], point[2]);
                point+=BOctTree<T>::POINTDIM;
              }
            } else {                         // select points to show
              // TODO smarter subselection of points here
              T each = (T)BOctTree<T>::POINTDIM * (T)((T)length/(T)newtargetpts);
              T *p;
              int index;
              for(unsigned int iterator = 0; iterator < newtargetpts; iterator++ ) {
                index = (T)iterator * each;
                p = point + index - index%BOctTree<T>::POINTDIM;
                if(cm) cm->setColor(p);
                glVertex3f( p[0], p[1], p[2]);
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

  void displayOctTreeLOD(long targetpts, bitoct<T> &node, T *center, T size ) {
    if (targetpts <= 0) return; // no need to display anything

    T ccenter[3];
    bitunion<T> *children;
    bitoct<T>::getChildren(node, children);

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
          pointrep *points = children->points;
          unsigned int length = points[0].length;
          T *point = &(points[1].v);  // first point
          glBegin(GL_POINTS);
          if (length > 10 && !LOD(ccenter[0], ccenter[1], ccenter[2], size/2.0) ) {  // only a single pixel on screen only paint one point
            if(cm) cm->setColor(point);
            glVertex3f( point[0], point[1], point[2]);
          } else if (length <= newtargetpts) {        // more points requested than possible, plot all
            for(unsigned int iterator = 0; iterator < length; iterator++ ) {
              if(cm) cm->setColor(point);
              glVertex3f( point[0], point[1], point[2]);
              point+=BOctTree<T>::POINTDIM;
            }
          } else {                         // select points to show
            // TODO smarter subselection of points here
            T each = (T)BOctTree<T>::POINTDIM * (T)((T)length/(T)newtargetpts);
            T *p;
            int index;
            for(unsigned int iterator = 0; iterator < newtargetpts; iterator++ ) {
              index = (T)iterator * each;
              p = point + index - index%BOctTree<T>::POINTDIM;
              if(cm) cm->setColor(p);
              glVertex3f( p[0], p[1], p[2]);
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

  void selectRay(vector<T *> &selpoints, bitoct<T> &node, T *center, T size) {
    if (!HitBoundingBox(center, size ))return;

    T ccenter[3];
    bitunion<T> *children;
    bitoct<T>::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          // check if leaf is visible
          if ( HitBoundingBox(ccenter, size) ) {
            pointrep *points = children->points;
            unsigned int length = points[0].length;
            T *point = &(points[1].v);  // first point
            for(unsigned int iterator = 0; iterator < length; iterator++ ) {
              selpoints.push_back(point);
              point+=BOctTree<T>::POINTDIM;
            }
          }
        } else { // recurse
          selectRay( selpoints, children->node, ccenter, size/2.0);
        }
        ++children; // next child
      }
    }
  }

  void selectRay(T * &selpoint, bitoct<T> &node, T *center, T size, float min) {
    if (!HitBoundingBox(center, size ))return;

    T ccenter[3];
    bitunion<T> *children;
    bitoct<T>::getChildren(node, children);

    for (short i = 0; i < 8; i++) {
      if (  ( 1 << i ) & node.valid ) {   // if ith node exists
        childcenter(center, ccenter, size, i);  // childrens center
        if (  ( 1 << i ) & node.leaf ) {   // if ith node is leaf get center
          // check if leaf is visible
          if ( HitBoundingBox(ccenter, size) ) {
            pointrep *points = children->points;
            unsigned int length = points[0].length;
            T *point = &(points[1].v);  // first point
            for(unsigned int iterator = 0; iterator < length; iterator++ ) {
              if (min > RayDist(point) && ScreenDist(point) < 5) {
                selpoint = point;
                min = RayDist(point);
              }
              point+=BOctTree<T>::POINTDIM;
            }
          }
        } else { // recurse
          selectRay( selpoint, children->node, ccenter, size/2.0, min);
        }
        ++children; // next child
      }
    }
  }

  ColorManager<T> *cm;
};

#endif
