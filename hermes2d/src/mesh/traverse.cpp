// This file is part of Hermes2D.
//
// Hermes2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D.  If not, see <http://www.gnu.org/licenses/>.

#include "mesh.h"
#include "traverse.h"
#include "mesh_function.h"

namespace Hermes
{
  namespace Hermes2D
  {
    static const Rect H2D_UNITY = { 0, 0, ONE, ONE };
    Traverse::Traverse(int spaces_size) : spaces_size(spaces_size)
    {
    }

    static int get_split_and_sons(Element* e, Rect* cr, Rect* er, int4& sons)
    {
      uint64_t hmid = (er->l + er->r) >> 1;
      uint64_t vmid = (er->t + er->b) >> 1;

      if (e->bsplit())
      {
        if (cr->r <= hmid && cr->t <= vmid)
          return (sons[0] = sons[1] = sons[2] = sons[3] = 0), 0;
        else if (cr->l >= hmid && cr->t <= vmid)
          return (sons[0] = sons[1] = sons[2] = sons[3] = 1), 0;
        else if (cr->l >= hmid && cr->b >= vmid)
          return (sons[0] = sons[1] = sons[2] = sons[3] = 2), 0;
        else if (cr->r <= hmid && cr->b >= vmid)
          return (sons[0] = sons[1] = sons[2] = sons[3] = 3), 0;
        else if (cr->r <= hmid)
          return (sons[0] = sons[1] = 0, sons[2] = sons[3] = 3), 1;
        else if (cr->l >= hmid)
          return (sons[0] = sons[1] = 1, sons[2] = sons[3] = 2), 1;
        else if (cr->t <= vmid)
          return (sons[0] = sons[3] = 0, sons[1] = sons[2] = 1), 2;
        else if (cr->b >= vmid)
          return (sons[0] = sons[3] = 3, sons[1] = sons[2] = 2), 2;
        else
          return (sons[0] = 0, sons[1] = 1, sons[2] = 2, sons[3] = 3), 3;
      }
      else if (e->hsplit())
      {
        if (cr->t <= vmid)
          return (sons[0] = sons[1] = sons[2] = sons[3] = 4), 0;
        else if (cr->b >= vmid)
          return (sons[0] = sons[1] = sons[2] = sons[3] = 5), 0;
        else
          return (sons[0] = sons[1] = 4, sons[2] = sons[3] = 5), 1;
      }
      else // e->vsplit()
      {
        if (cr->r <= hmid)
          return (sons[0] = sons[1] = sons[2] = sons[3] = 6), 0;
        else if (cr->l >= hmid)
          return (sons[0] = sons[1] = sons[2] = sons[3] = 7), 0;
        else
          return (sons[0] = sons[3] = 6, sons[1] = sons[2] = 7), 2;
      }
    }

    static void move_to_son(Rect* rnew, Rect* rold, int son)
    {
      uint64_t hmid = (rold->l + rold->r) >> 1;
      uint64_t vmid = (rold->t + rold->b) >> 1;
      if (rnew != rold)
        memcpy(rnew, rold, sizeof(Rect));

      switch (son)
      {
      case 0: rnew->r = hmid; rnew->t = vmid; break;
      case 1: rnew->l = hmid; rnew->t = vmid; break;
      case 2: rnew->l = hmid; rnew->b = vmid; break;
      case 3: rnew->r = hmid; rnew->b = vmid; break;
      case 4: rnew->t = vmid; break;
      case 5: rnew->b = vmid; break;
      case 6: rnew->r = hmid; break;
      case 7: rnew->l = hmid; break;
      }
    }

    void Traverse::init_transforms(Traverse::State* s, unsigned char i)
    {
      Rect r;
      memcpy(&r, s->er + i, sizeof(Rect));

      while (s->cr.l > r.l || s->cr.r < r.r || s->cr.b > r.b || s->cr.t < r.t)
      {
        uint64_t hmid = (r.l + r.r) >> 1;
        uint64_t vmid = (r.t + r.b) >> 1;
        int son;

        if (s->cr.r <= hmid && s->cr.t <= vmid) son = 0;
        else if (s->cr.l >= hmid && s->cr.t <= vmid) son = 1;
        else if (s->cr.l >= hmid && s->cr.b >= vmid) son = 2;
        else if (s->cr.r <= hmid && s->cr.b >= vmid) son = 3;
        else if (s->cr.r <= hmid) son = 6;
        else if (s->cr.l >= hmid) son = 7;
        else if (s->cr.t <= vmid) son = 4;
        else if (s->cr.b >= vmid) son = 5;
        else assert(0);

        s->push_transform(son, i, s->is_triangle());
        move_to_son(&r, &r, son);
      }
    }

    Traverse::State::State()
    {
      memset(this, 0, sizeof(Traverse::State));
      for (int i = 0; i < 4; i++)
        this->bnd[i] = true;
      cr = H2D_UNITY;
      for (int i = 0; i < num; i++)
        er[i] = H2D_UNITY;
      isurf = -1;
    }

    /*
    void Traverse::State::operator=(const State * other)
    {
    // Delete part.
    if(e != nullptr)
    delete [] e;
    if(sub_idx != nullptr)
    delete [] sub_idx;

    this->num = other->num;

    this->e = new Element*[num];
    this->sub_idx = new uint64_t[num];
    memcpy(this->e, other->e, num * sizeof(Element*));
    memcpy(this->sub_idx, other->sub_idx, num * sizeof(uint64_t));
    memcpy(this->bnd, other->bnd, 4 * sizeof(bool));

    this->rep = other->rep;
    this->visited = other->visited;
    this->isurf = other->isurf;
    this->isBnd = other->isBnd;
    }
    */
    Traverse::State* Traverse::State::clone(const Traverse::State * other)
    {
      State* state = new State();

      state->num = other->num;

      state->e = new Element*[state->num];
      state->sub_idx = new uint64_t[state->num];
      memcpy(state->e, other->e, state->num * sizeof(Element*));
      memcpy(state->sub_idx, other->sub_idx, state->num * sizeof(uint64_t));
      memcpy(state->bnd, other->bnd, 4 * sizeof(bool));

      state->rep = other->rep;
      state->rep_i = other->rep_i;
      state->visited = other->visited;
      state->isurf = other->isurf;
      state->isBnd = other->isBnd;

      return state;
    }

    Traverse::State::~State()
    {
      if (e != nullptr)
        delete[] e;
      if (sub_idx != nullptr)
        delete[] sub_idx;
    }

    void Traverse::State::push_transform(unsigned char son, unsigned char i, bool is_triangle)
    {
      this->sub_idx[i] = (sub_idx[i] << 3) + son + 1;

      if (is_triangle)
      {
        if (son < 3)
        {
          switch (son)
          {
          case 0: bnd[1] = false; break;
          case 1: bnd[2] = false; break;
          case 2: bnd[0] = false; break;
          }
        }
        else
        {
          memset(bnd, 0, sizeof(bnd));
        }
      }
      else
      {
        if (son != 0 && son != 1 && son != 4 && son != 6 && son != 7)
          bnd[0] = false;
        if (son != 1 && son != 2 && son != 7 && son != 4 && son != 5)
          bnd[1] = false;
        if (son != 2 && son != 3 && son != 5 && son != 6 && son != 7)
          bnd[2] = false;
        if (son != 3 && son != 0 && son != 6 && son != 4 && son != 5)
          bnd[3] = false;
      }
    }

    uint64_t Traverse::State::get_transform(unsigned char i)
    {
      return this->sub_idx[i];
    }

    Traverse::State* Traverse::push_state(int* top_by_ref)
    {
      int* top_f = (top_by_ref == nullptr) ? &this->top : top_by_ref;

      if (*top_f >= size)
        throw Hermes::Exceptions::Exception("Stack overflow. Increase stack size.");

      if (stack[*top_f].e == nullptr)
      {
        stack[*top_f].e = new Element*[num];
        stack[*top_f].er = new Rect[num];
        stack[*top_f].sub_idx = new uint64_t[num];
      }

      stack[*top_f].visited = false;
      stack[*top_f].isurf = -1;
      memset(stack[*top_f].sub_idx, 0, num * sizeof(uint64_t));
      for (int i = 0; i < 4; i++)
        stack[*top_f].bnd[i] = true;
      stack[*top_f].num = this->num;

      return stack + (*top_f)++;
    }

    void Traverse::set_boundary_info(State* s)
    {
      Element* e = nullptr;
      for (int i = 0; i < num; i++)
        if ((e = s->e[i]) != nullptr) break;

      if (e->is_triangle())
      {
        for (int i = 0; i < 3; i++)
          (s->bnd[i] = (s->bnd[i] && e->en[i]->bnd));
        s->isBnd = s->bnd[0] || s->bnd[1] || s->bnd[2] || e->vn[0]->bnd || e->vn[1]->bnd || e->vn[2]->bnd;
      }
      else
      {
        s->bnd[0] = s->bnd[0] && (s->cr.b == 0) && e->en[0]->bnd;
        s->bnd[1] = s->bnd[1] && (s->cr.r == ONE) && e->en[1]->bnd;
        s->bnd[2] = s->bnd[2] && (s->cr.t == ONE) && e->en[2]->bnd;
        s->bnd[3] = s->bnd[3] && (s->cr.l == 0) && e->en[3]->bnd;
        s->isBnd = s->bnd[0] || s->bnd[1] || s->bnd[2] || s->bnd[3] || e->vn[0]->bnd || e->vn[1]->bnd || e->vn[2]->bnd || e->vn[3]->bnd;
      }
    }

    bool Traverse::State::is_triangle()
    {
      bool is_triangle = false;
      for (int j = 0; j < this->num; j++)
      {
        if (this->e[j] != nullptr)
        {
          if (this->e[j]->is_triangle())
            is_triangle = true;
          break;
        }
      }
      return is_triangle;
    }

    template<typename Scalar>
    Traverse::State** Traverse::get_states(std::vector<MeshFunctionSharedPtr<Scalar> > mesh_functions, unsigned int& states_count)
    {
      std::vector<MeshSharedPtr> meshes;
      for (unsigned short i = 0; i < mesh_functions.size(); i++)
        meshes.push_back(mesh_functions[i]->get_mesh());
      return this->get_states(meshes, states_count);
    }

    Traverse::State** Traverse::get_states(std::vector<MeshSharedPtr> meshes, unsigned int& states_count)
    {
      return Traverse::get_states(&meshes[0], meshes.size(), states_count);
    }

    Traverse::State** Traverse::get_states(MeshSharedPtr* meshes, unsigned short meshes_count, unsigned int& states_count)
    {
      // This will be returned.
      int count = 0, predictedCount = 0;
      this->num = meshes_count;
      for (int i = 0; i < meshes_count; i++)
        if (meshes[i]->get_num_active_elements() > predictedCount)
          predictedCount = meshes[i]->get_num_active_elements();
      State** states = malloc_with_check<State*>(predictedCount);

      this->begin(num);

      int id = 0;

      while (1)
      {
        int i, son;
        // When the stack of states is not empty (i.e. not at the beginning) the function starts here.
        // If the top state was visited already, we are returning through it:
        // undo all its transformations, pop it and continue with a non-visited one
        State* s;
        while (top > 0 && (s = stack + top - 1)->visited)
          (top)--;

        // The stack is empty, take next base element
        // The process starts here (at the beginning the stack is always empty, i.e. top == 0)
        if (top <= 0)
        {
          // Push the state of a new_ base element.
          // This function only allocates memory for the new_ state,
          // with as many Elements* as there are meshes in this stage.
          s = push_state();
          s->cr = H2D_UNITY;
          while (1)
          {
            // No more base elements? we're finished.
            // Id is set to zero at the beginning by the function trav.begin(..).
            if (id >= meshes[0]->get_num_base_elements())
            {
              this->finish();
              states_count = count;
              return states;
            }

            int nused = 0;
            // The variable num is the number of meshes in the stage
            for (i = 0; i < num; i++)
            {
              // Retrieve the Element with this id on the i-th mesh.
              s->e[i] = meshes[i]->get_element(id);
              if (!s->e[i]->used)
              {
                s->e[i] = nullptr;
                continue;
              }
              else
              {
                s->rep = s->e[i];
                s->rep_i = 0;
              }
              s->er[i] = H2D_UNITY;
              nused++;
            }
            // If there is any used element in this stage we continue with the calculation
            // (break this cycle looking for such an element id).
            if (nused)
              break;
            (id)++;
          }

          (id)++;

          if (s->is_triangle())
            for (i = 0; i < 3; i++)
              s->bnd[i] = true;
        }

        // Entering a new_ state, perform transformations.
        s->visited = true;
        for (i = 0; i < num; i++)
        {
          // ..where the element is used ..
          if (s->e[i] != nullptr && s->e[i]->used)
            if (s->sub_idx[i] == 0 && s->e[i]->active)
              if (!s->e[i]->is_triangle())
                init_transforms(s, i);
        }

        // Is this the leaf state?
        bool leaf = true;
        for (i = 0; i < num; i++)
        {
          if (s->e[i] != nullptr && s->e[i]->used)
            if (!s->e[i]->active)
            {
            leaf = false;
            break;
            }
        }

        // if yes, set boundary flags and return the state
        if (leaf)
        {
          if (count > predictedCount - 1)
          {
            predictedCount *= 1.5;
            states = realloc_with_check<State*>(states, predictedCount);
          }

          set_boundary_info(s);
          s->rep = nullptr;
          // EXTREMELY IMPORTANT.
          // The for loop here is NOT through all meshes, but only
          // through the spaces.
          // The reason is not to include states that only have elements
          // on meshes that are not a part of the weak form.
          for (int j = 0; j < this->spaces_size; j++)
            if (s->e[j] != nullptr && s->e[j]->used)
            {
            s->rep = s->e[j];
            s->rep_i = j;
            }
          if (s->rep)
            states[count++] = State::clone(s);
          continue;
        }

        // Triangle: push son states
        if (s->is_triangle())
        {
          // Triangle always has 4 sons.
          for (son = 0; son <= 3; son++)
          {
            State* ns = push_state();
            // For every mesh..
            for (i = 0; i < num; i++)
            {
              // ..if the element is not used.
              if (s->e[i] == nullptr || !s->e[i]->used)
              {
                ns->e[i] = nullptr;
              }
              else if (s->e[i]->active)
              {
                ns->e[i] = s->e[i];
                ns->sub_idx[i] = s->sub_idx[i];
                ns->push_transform(son, i, true);
              }
              // ..we move to the son.
              else
              {
                ns->e[i] = s->e[i]->sons[son];
                // If the son's element is active.
                if (ns->e[i]->active)
                  ns->sub_idx[i] = 0;
              }
            }

            // Determine boundary flags and positions for the new_ state.
            if (son < 3)
            {
              memcpy(ns->bnd, s->bnd, sizeof(ns->bnd));

              switch (son)
              {
              case 0: ns->bnd[1] = false; break;
              case 1: ns->bnd[2] = false; break;
              case 2: ns->bnd[0] = false; break;
              }
            }
            else
            {
              memset(ns->bnd, 0, sizeof(ns->bnd));
            }
          }
        }
        // Quad: this is a little more complicated, same principle, though.
        else
        {
          // Obtain split types and son numbers for the current rectangle on all elements.
          int4* current_sons = new int4[num];
          int split = 0;
          for (i = 0; i < num; i++)
            if (s->e[i] != nullptr && !s->e[i]->active)
              split |= get_split_and_sons(s->e[i], &s->cr, s->er + i, current_sons[i]);

          // Both splits: recur to four sons, similar to triangles.
          if (split == 3)
          {
            for (son = 0; son <= 3; son++)
            {
              State* ns = push_state();
              // Sets the son's "base" rectangle to the correct one.
              move_to_son(&ns->cr, &s->cr, son);

              for (i = 0; i < num; i++)
              {
                if (s->e[i] == nullptr || !s->e[i]->used)
                {
                  ns->e[i] = nullptr;
                }
                else
                {
                  if (s->e[i]->active)
                  {
                    ns->e[i] = s->e[i];
                    ns->sub_idx[i] = s->sub_idx[i];
                    ns->push_transform(son, i);
                  }
                  else
                  {
                    ns->e[i] = s->e[i]->sons[current_sons[i][son] & 3];
                    // Sets the son's "current mesh" rectangle correctly.
                    move_to_son(ns->er + i, s->er + i, current_sons[i][son]);
                    if (ns->e[i]->active)
                      ns->sub_idx[i] = 0;
                  }
                }
              }
            }
          }
          // V or h split, recur to two sons.
          else if (split > 0)
          {
            int son0 = 4, son1 = 5;
            if (split == 2) { son0 = 6; son1 = 7; }

            for (son = son0; son <= son1; son++)
            {
              State* ns = push_state();
              move_to_son(&ns->cr, &s->cr, son);

              int j = (son == 4 || son == 6) ? 0 : 2;
              for (i = 0; i < num; i++)
              {
                if (s->e[i] == nullptr || !s->e[i]->used)
                {
                  ns->e[i] = nullptr;
                }
                else
                {
                  if (s->e[i]->active)
                  {
                    ns->e[i] = s->e[i];
                    ns->sub_idx[i] = s->sub_idx[i];
                    ns->push_transform(son, i);
                  }
                  else
                  {
                    ns->e[i] = s->e[i]->sons[current_sons[i][j] & 3];
                    move_to_son(ns->er + i, s->er + i, current_sons[i][j]);
                    if (ns->e[i]->active)
                      ns->sub_idx[i] = 0;
                  }
                }
              }
            }
          }

          // No splits, recur to one son.
          else
          {
            State* ns = push_state();
            memcpy(&ns->cr, &s->cr, sizeof(Rect));

            for (i = 0; i < num; i++)
            {
              if (s->e[i] == nullptr || !s->e[i]->used)
              {
                ns->e[i] = nullptr;
              }
              else if (s->e[i]->active)
              {
                ns->e[i] = s->e[i];
                memcpy(&ns->er[i], &ns->cr, sizeof(Rect));
              }
              else
              {
                ns->e[i] = s->e[i]->sons[current_sons[i][0] & 3];
                move_to_son(ns->er + i, s->er + i, current_sons[i][0]);
                if (ns->e[i]->active)
                  ns->sub_idx[i] = 0;
              }
            }
          }
          delete[] current_sons;
        }
      }
      this->finish();
    }

    static void testMeshesCompliance(int n, MeshSharedPtr* meshes)
    {
      // Test whether all master meshes have the same number of elements.
      int base_elem_num = meshes[0]->get_num_base_elements();
      for (int i = 1; i < n; i++)
        if (base_elem_num != meshes[i]->get_num_base_elements())
          throw Hermes::Exceptions::Exception("Meshes not compatible in Traverse::begin().");
    }

    static void testMeshesQuality(int n, MeshSharedPtr* meshes)
    {
      Element* e;
      int base_elem_num = meshes[0]->get_num_base_elements();
      // Test whether areas of corresponding elements are the same.
      double *areas = new double[base_elem_num];
      memset(areas, 0, base_elem_num*sizeof(double));

      // Read base element areas from the first mesh,
      // Also get minimum element area.
      int counter = 0;
      double min_elem_area = 1e30;
      for_all_base_elements_incl_inactive(e, meshes[0])
      {
        if (!e->used)
          areas[counter] = 0.0;
        else
        {
          areas[counter] = e->area;
          if (areas[counter] < min_elem_area)
            min_elem_area = areas[counter];
        }

        counter++;
      }
      // take one mesh at a time and compare element areas to the areas[] array
      double tolerance = min_elem_area / 100.;

      if (min_elem_area < 0)
        throw Exceptions::ValueException("min_elem_area", 0.0, Hermes::HermesSqrtEpsilon);

      for (int i = 1; i < n; i++)
      {
        counter = 0;
        for_all_base_elements_incl_inactive(e, meshes[i])
        {
          if (e->used)
            if (fabs(areas[counter] - e->area) > tolerance && areas[counter] > Hermes::HermesSqrtEpsilon)
            {
            throw Hermes::Exceptions::Exception("An element is probably too distorted, try different meshing.");
            }
          counter++;
        }
      }
      delete[] areas;
    }

    void Traverse::begin(int n)
    {
      assert(n > 0);
      num = n;

      size = 256;

      stack = new State[size];
      memset(stack, 0, size * sizeof(State));

      top = 0;
    }

    void Traverse::free_state(Traverse::State* state)
    {
      delete[] state->e;
      delete[] state->er;
      delete[] state->sub_idx;
      memset(state, 0, sizeof(Traverse::State));
    }

    void Traverse::finish()
    {
      if (stack == nullptr)
        return;

      for (int i = 0; i < size; i++)
        if (stack[i].e != nullptr)
          free_state(stack + i);

      delete[] stack;
      stack = nullptr;
    }

    uint64_t Traverse::init_idx(Rect* cr, Rect* er)
    {
      Rect r;
      memcpy(&r, er, sizeof(Rect));

      uint64_t idx = 0;
      while (cr->l > r.l || cr->r < r.r || cr->b > r.b || cr->t < r.t)
      {
        uint64_t hmid = (r.l + r.r) >> 1;
        uint64_t vmid = (r.t + r.b) >> 1;
        int son = -1;

        if (cr->r <= hmid && cr->t <= vmid) { son = 0; r.r = hmid; r.t = vmid; }
        else if (cr->l >= hmid && cr->t <= vmid) { son = 1; r.l = hmid; r.t = vmid; }
        else if (cr->l >= hmid && cr->b >= vmid) { son = 2; r.l = hmid; r.b = vmid; }
        else if (cr->r <= hmid && cr->b >= vmid) { son = 3; r.r = hmid; r.b = vmid; }
        else if (cr->t <= vmid) { son = 4; r.t = vmid; }
        else if (cr->b >= vmid) { son = 5; r.b = vmid; }
        else if (cr->r <= hmid) { son = 6; r.r = hmid; }
        else if (cr->l >= hmid) { son = 7; r.l = hmid; }
        else assert(0);

        idx = (idx << 3) + son + 1;
      }
      return idx;
    }

    void Traverse::union_recurrent(Rect* cr, Element** e, Rect* er, uint64_t* idx, Element* uni)
    {
      int i, j, son;

      // are we at the bottom?
      bool leaf = true;
      for (i = 0; i < num; i++)
      {
        if (!e[i]->active)
        {
          leaf = false;
          break;
        }
      }
      // if yes, store the element transformation indices
      if (leaf)
      {
        if (udsize <= uni->id)
        {
          if (!udsize) udsize = 1024;
          while (udsize <= uni->id)
            udsize *= 2;
          for (i = 0; i < num; i++)
            unidata[i] = (UniData*)realloc(unidata[i], udsize * sizeof(UniData));
        }
        for (i = 0; i < num; i++)
        {
          unidata[i][uni->id].e = e[i];
          unidata[i][uni->id].idx = idx[i];
        }
        return;
      }

      // state arrays
      Element** e_new = new Element*[num];
      Rect* er_new = new Rect[num];
      Rect cr_new;

      int4* sons = new int4[num];
      uint64_t* idx_new = new uint64_t[num];
      memcpy(idx_new, idx, num*sizeof(uint64_t));

      if (uni->is_triangle())
      {
        // visit all sons of the triangle
        unimesh->refine_element_id(uni->id);
        for (son = 0; son <= 3; son++)
        {
          for (i = 0; i < num; i++)
          {
            if (e[i]->active)
            {
              e_new[i] = e[i];
              idx_new[i] = (idx[i] << 3) + son + 1;
            }
            else
              e_new[i] = e[i]->sons[son];
          }
          union_recurrent(nullptr, e_new, nullptr, idx_new, uni->sons[son]);
        }
      }
      else
      {
        // obtain split types and son numbers for the current rectangle on all elements
        int split = 0;
        for (i = 0; i < num; i++)
          if (!e[i]->active)
            split |= get_split_and_sons(e[i], cr, er + i, sons[i]);

        // both splits: recur to four sons
        if (split == 3)
        {
          unimesh->refine_element_id(uni->id, 0);

          for (son = 0; son <= 3; son++)
          {
            move_to_son(&cr_new, cr, son);
            for (i = 0; i < num; i++)
            {
              if (e[i]->active)
              {
                e_new[i] = e[i];
                idx_new[i] = (idx[i] << 3) + son + 1;
              }
              else
              {
                e_new[i] = e[i]->sons[sons[i][son] & 3];
                move_to_son(&(er_new[i]), er + i, sons[i][son]);
                if (e_new[i]->active)
                  idx_new[i] = init_idx(&cr_new, &(er_new[i]));
              }
            }
            union_recurrent(&cr_new, e_new, er_new, idx_new, uni->sons[son]);
          }
        }
        // v or h split, recur to two sons
        else if (split > 0)
        {
          unimesh->refine_element_id(uni->id, split);

          int son0 = 4, son1 = 5;
          if (split == 2) { son0 = 6; son1 = 7; }

          for (son = son0; son <= son1; son++)
          {
            move_to_son(&cr_new, cr, son);
            j = (son == 4 || son == 6) ? 0 : 2;
            for (i = 0; i < num; i++)
            {
              if (e[i]->active)
              {
                e_new[i] = e[i];
                idx_new[i] = (idx[i] << 3) + son + 1;
              }
              else
              {
                e_new[i] = e[i]->sons[sons[i][j] & 3];
                move_to_son(&(er_new[i]), er + i, sons[i][j]);
                if (e_new[i]->active)
                  idx_new[i] = init_idx(&cr_new, &(er_new[i]));
              }
            }
            union_recurrent(&cr_new, e_new, er_new, idx_new, uni->sons[son & 3]);
          }
        }
        // no splits, recur to one son
        else
        {
          memcpy(&cr_new, cr, sizeof(Rect));
          for (i = 0; i < num; i++)
          {
            if (e[i]->active)
              e_new[i] = e[i];
            else
            {
              e_new[i] = e[i]->sons[sons[i][0] & 3];
              move_to_son(&(er_new[i]), er + i, sons[i][0]);
              if (e_new[i]->active)
                idx_new[i] = init_idx(&cr_new, &(er_new[i]));
            }
          }
          union_recurrent(&cr_new, e_new, er_new, idx_new, uni);
        }
      }

      delete[] e_new;
      delete[] er_new;
      delete[] sons;
      delete[] idx_new;
    }

    UniData** Traverse::construct_union_mesh(unsigned char n, MeshSharedPtr* meshes, MeshSharedPtr unimesh)
    {
      // Initial check.
      testMeshesCompliance(n, meshes);

      Traverse traverse(n);

      // Initialization.
      traverse.begin(n);

      int i;
      Element** e = new Element*[n];
      Rect* er = new Rect[n];
      Rect cr;

      traverse.unimesh = unimesh;
      unimesh->copy_base(meshes[0]);

      // Unimesh initialization.
      traverse.udsize = 0;
      traverse.unidata = new UniData*[n];
      memset(traverse.unidata, 0, sizeof(UniData*)* n);

      uint64_t* idx = new uint64_t[n];
      memset(idx, 0, n*sizeof(uint64_t));

      // Calculation.
      for (int id = 0; id < meshes[0]->get_num_base_elements(); id++)
      {
        if (!meshes[0]->get_element(id)->used)
          continue;
        for (i = 0; i < n; i++)
        {
          e[i] = meshes[i]->get_element(id);
          static const Rect H2D_UNITY = { 0, 0, ONE, ONE };
          cr = er[i] = H2D_UNITY;
        }
        traverse.union_recurrent(&cr, e, er, idx, unimesh->get_element(id));
      }

      delete[] e;
      delete[] er;
      delete[] idx;

      traverse.finish();
      return traverse.unidata;
    }

    template HERMES_API Traverse::State** Traverse::get_states<double>(std::vector<MeshFunctionSharedPtr<double> > mesh_functions, unsigned int& states_count);
    template HERMES_API Traverse::State** Traverse::get_states<std::complex<double> >(std::vector<MeshFunctionSharedPtr<std::complex<double> > > mesh_functions, unsigned int& states_count);
  }
}