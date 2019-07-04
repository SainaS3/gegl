/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2018, 2019 Øyvind Kolås <pippin@gimp.org>
 *
 */

#include <stdio.h>
#include "config.h"
#include <glib/gi18n-lib.h>
#include <malloc.h>


#ifdef GEGL_PROPERTIES

property_int (seek_distance, "seek radius", 9)
  description ("Maximum distance in neighborhood we look for better candidates per improvement.")
  value_range (1, 512)

property_double (seek_reduction, "seek reduction", 0.9)
  description ("factor seek distance is shortened, until we're about 2px short per iteration, 1.0 means no reduction")
  value_range (0.0, 1.0)

property_int (min_iter, "min runs", 100)
  description ("Ensuring that we get results even with low retry chance")
  value_range (1, 512)

property_int (max_iter, "max runs", 200)
  description ("Mostly a saftey valve, so that we terminate")
  value_range (1, 40000)

property_int (iterations, "iterations per round per probe", 10)
  description ("number of improvement iterations, after initial search - that each probe gets.")
  value_range (1, 1000)

property_int (rounds, "rounds", 32)
  description ("number of improvement iterations, after initial search - that each probe gets.")
  value_range (1, 1000)

property_double (chance_try, "try probability", 0.25)
  description ("The chance that a candidate pixel probe will start being filled in")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.1)
property_double (chance_retry, "retry probability", 0.8)
  description ("The chance that a pixel probe gets an improvement in an iteration")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.1)

property_double (metric_dist_powk, "metric dist powk", 2.0)
  description ("influences the lack of importance of further away pixels")
  value_range (0.0, 10.0)
  ui_steps    (0.1, 1.0)

property_double (metric_empty_hay_score, "metric empty hay score", 0.44)
  description ("score given to pixels that are empty, in the search neighborhood of pixel, this being at default or higher value sometimes discourages some of the good very nearby matches")
  value_range (0.01, 100.0)
  ui_steps    (0.05, 0.1)

property_double (metric_empty_needle_score, "metric empty needle score", 0.2)
  description ("the score given in the metric to an empty spot")
  value_range (0.01, 100.0)
  ui_steps    (0.05, 0.1)

property_double (metric_cohesion, "metric cohesion", 0.01)
  description ("influences the importance of probe spatial proximity")
  value_range (0.0, 100.0)
  ui_steps    (0.2, 0.2)

property_double (ring_twist, "ring twist", 0.0)
  description ("incremental twist per circle, in radians")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.2)

property_double (ring_gap1,    "ring gap1", 1.3)
  description ("radius, in pixels of nearest to pixel circle of neighborhood metric")
  value_range (0.0, 16.0)
  ui_steps    (0.25, 0.25)

property_double (ring_gap2,    "ring gap2", 2.5)
  description ("radius, in pixels of second nearest to pixel circle")
  value_range (0.0, 16.0)
  ui_steps    (0.25, 0.25)

property_double (ring_gap3,    "ring gap3", 3.7)
  description ("radius, in pixels of third pixel circle")
  value_range (0.0, 16.0)
  ui_steps    (0.25, 0.25)

property_double (ring_gap4, "ring gap4", 5.5)
  description ("radius, in pixels of fourth pixel circle (not always in use)")
  value_range (0.0, 16.0)
  ui_steps    (0.25, 0.25)


#else


/* content aware pixel-neighborhood filler
 *
 * For performance it now works by creating probes for candidate pixels for
 * infilling and iteratively searching with a shrinking neighborhood for better
 * matching pixels - using a random spray araound the best result found; by
 * starting in the neighborhood to be filled in we are likely to find a good
 * match for a hole in a texture.
 *
 * The quality of a match is determined by the squared color difference, scaled
 * by distance to center of sampling. Combined with a cohesion factor that
 * gives weight to probes that are sampling from near the same location.
 */

#define RINGS                   3   // increments works up to 7-8 with no adver
#define RAYS                   12  // good values for testing 6 8 10 12 16
#define NEIGHBORHOOD            (RINGS*RAYS+1)

/* The pattern of the sampling neighborhood is RAYS of samples radiating out
 * from the sample point forming a set of concentric circles. We sample this
 * pattern once per pixel using cubic sampler. The circles of rays are rotated
 * after sampling so that the ray with the most energy is stored first - this
 * is how we achieve orientation invariance.
 */
#define DIRECTION_INVARIANT // comment out to make search be direction dependent

#define N_SCALE_NEEDLES         3

/* Before comparing with a candidate extracted hay-feature, we prepare
 * ourselves with N_SCALE_NEEDLES independent scaled versions of the
 * destination pixel neighborhood. This scale invariance further multiplying
 * the various permutations of variants of a neighborhood that we match to 12*3
 * = 36 times.
 */

#define GET_INSPIRED_BY_NEIGHBORS

/* Before looking for matches in our own neighborhood we look if any good match
 * in the 8 nearest neighboring probes, neighboring pixels. This selection
 * criteria helps aid texture building when this remains the best match.
 */


#define MAX_PROBES 1000000


#define GEGL_OP_FILTER
#define GEGL_OP_NAME      alpha_inpaint
#define GEGL_OP_C_SOURCE  alpha-inpaint.c

#include "gegl-op.h"
#include <stdio.h>

#include <math.h>

#define POW2(x) ((x)*(x))

#define INITIAL_SCORE 1200000000

typedef struct
{
  GeglOperation *op;
  GeglProperties *o;
  GeglBuffer    *reference;
  GeglBuffer    *input;
  GeglBuffer    *output;
  GeglRectangle  in_rect;
  GeglRectangle  out_rect;
  GeglSampler   *in_sampler_f;
  GeglSampler   *ref_sampler_f;
  GeglSampler   *out_sampler_f;
  const Babl    *format; /* RGBA float in right space */

  float          ring_gaps[8];

  GHashTable    *ht[1];

  GHashTable    *probes_ht;

  float          min_x;
  float          min_y;
  float          max_x;
  float          max_y;

  float          order[512][3];
} PixelDuster;



typedef struct _Probe Probe;


typedef float   needles_t[N_SCALE_NEEDLES][4 * NEIGHBORHOOD ];// should be on stack


struct _Probe {
  int     target_x;
  int     target_y;
  int     age;              // not really needed
  float   score;
  int     source_x;
  int     source_y;
};

/* used for hash-table keys */
#define xy2offset(x,y)   GINT_TO_POINTER(((y) * 65536 + (x)))


/* when going through the image preparing the index - only look at the subset
 * needed pixels - and later when fetching out hashed pixels - investigate
 * these ones in particular. would only be win for limited inpainting..
 *
 * making the pixel duster scale invariant on a subpixel level would be neat
 * especially for supersampling, taking the reverse jacobian into account
 * would be even neater.
 *
 */


static void init_order(PixelDuster *duster)
{
  int i;
  GeglProperties *o = duster->o;
  /* the first element extracted is the center pixel, it is not used since
     we refetch it, thus this is a potential 4x4 bytes of scratch header space

     (or we could use the color of the pixel itself as part of a color histogram
      metric)
   */
  duster->order[0][0] = 0;
  duster->order[0][1] = 0;
  duster->order[0][2] = 1.0;
  i = 1;

  for (int circleno = 1; circleno < RINGS; circleno++)
    for (float angleno = 0; angleno < RAYS; angleno++)
    {
      float mag = duster->ring_gaps[circleno];
      float x, y;

      x = cosf ((angleno / RAYS + o->ring_twist*circleno) * M_PI * 2) * mag;
      y = sinf ((angleno / RAYS + o->ring_twist*circleno) * M_PI * 2) * mag;
      duster->order[i][0] = x;
      duster->order[i][1] = y;
      duster->order[i][2] = powf (1.0 / (POW2(x)+POW2(y)), o->metric_dist_powk);
      i++;
    }
}

static void duster_idx_to_x_y (PixelDuster *duster, int index, float *x, float *y)
{
  *x =  duster->order[index][0];
  *y =  duster->order[index][1];
}


static PixelDuster * pixel_duster_new (GeglBuffer *reference,
                                       GeglBuffer *input,
                                       GeglBuffer *output,
                                       const GeglRectangle *in_rect,
                                       const GeglRectangle *out_rect,
                                       float       ring_gap1,
                                       float       ring_gap2,
                                       float       ring_gap3,
                                       float       ring_gap4,

                                       GeglOperation *op)
{
  PixelDuster *ret = g_malloc0 (sizeof (PixelDuster));
  ret->o = GEGL_PROPERTIES (op);
  ret->reference   = reference;
  ret->input       = input;
  ret->output      = output;
  ret->op = op;
  ret->ring_gaps[1] = ring_gap1;
  ret->ring_gaps[2] = ring_gap2;
  ret->ring_gaps[3] = ring_gap3;
  ret->ring_gaps[4] = ring_gap4;
  ret->max_x = 0;
  ret->max_y = 0;
  ret->min_x = 10000;
  ret->min_y = 10000;
  ret->format = babl_format_with_space ("RGBA float", gegl_buffer_get_format (ret->input));
  ret->in_rect  = *in_rect;
  ret->out_rect = *out_rect;

  ret->in_sampler_f = gegl_buffer_sampler_new (input,
                                               ret->format,
                                               GEGL_SAMPLER_LINEAR);

  ret->ref_sampler_f = gegl_buffer_sampler_new (reference,
                                               ret->format,
                                               GEGL_SAMPLER_LINEAR);

  ret->out_sampler_f = gegl_buffer_sampler_new (output,
                                               ret->format,
                                               GEGL_SAMPLER_LINEAR);

  for (int i = 0; i < 1; i++)
    ret->ht[i] = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
  ret->probes_ht = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
  init_order (ret);
  return ret;
}

static inline void pixel_duster_remove_probes (PixelDuster *duster)
{
  if (duster->probes_ht)
  {
    g_hash_table_destroy (duster->probes_ht);
    duster->probes_ht = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
  }
}

static void pixel_duster_destroy (PixelDuster *duster)
{
  pixel_duster_remove_probes (duster);
  for (int i = 0; i < 1; i++)
  {
    g_hash_table_destroy (duster->ht[i]);
  }

  g_object_unref (duster->ref_sampler_f);
  g_object_unref (duster->in_sampler_f);
  g_object_unref (duster->out_sampler_f);

  g_free (duster);
}


void gegl_sampler_prepare (GeglSampler *sampler);

/*  extend with scale factor/matrix
 *
 */
static void extract_site (PixelDuster *duster, GeglBuffer *buffer, double x, double y, float scale, gfloat *dst)
{
  GeglSampler *sampler_f;
  if (buffer == duster->output)
  {
    sampler_f = duster->out_sampler_f;
    gegl_sampler_prepare (sampler_f);
  }
  else if (buffer == duster->reference)
  {
    sampler_f   = duster->ref_sampler_f;
  }
  else if (buffer == duster->input)
  {
    sampler_f   = duster->in_sampler_f;
  }

  for (int i = 0; i < NEIGHBORHOOD; i++)
  {
    float dx, dy;
    duster_idx_to_x_y (duster, i, &dx, &dy);
    gegl_sampler_get (sampler_f, x + dx * scale, y + dy * scale, NULL, &dst[i*4], 0);
  }

#ifdef DIRECTION_INVARIANT
  {
    int warmest_ray = 0;
    float warmest_ray_energy = 0;
    gfloat tmp[4 * NEIGHBORHOOD];

    for (int ray = 0; ray < RAYS; ray ++)
    {
      float energy = 0.0;
      int count = 0;
      for (int circle = 0; circle < RINGS; circle++)
        if (dst[ ( circle * RAYS  + ray )*4 + 3] > 0.01)
        {
          for (int c = 0; c < 3; c++)
            energy += dst[ ( circle * RAYS  + ray )*4 + c];
          count ++;
        }
      if (count)
        energy/=count;
      if (energy > warmest_ray_energy)
      {
        warmest_ray = ray;
        warmest_ray_energy = energy;
      }
    }

    if (warmest_ray)
    {
      for (int i = 0; i < NEIGHBORHOOD*4; i++)
        tmp[i] = dst[i];

      for (int ray = 0; ray < RAYS; ray ++)
      {
        int swapped_ray = ray + warmest_ray;
        while (swapped_ray >= RAYS) swapped_ray -= RAYS;

        for (int circle = 0; circle < RINGS; circle++)
        for (int c = 0; c < 4; c++)
          dst[ ( circle * RAYS  + ray )*4 + c] =
          tmp[ ( circle * RAYS  + swapped_ray )*4 + c];
       }
    }
  }
#endif
}

static inline float f_rgb_diff (float *a, float *b)
{
  return POW2(a[0]-b[0]) + POW2(a[1]-b[1]) + POW2(a[2]-b[2]);
}

static float inline
score_site (PixelDuster *duster,
            Probe       *probe,
            Probe      **neighbors,
            int          x,
            int          y,
            gfloat      *needle,
            gfloat      *hay,
            float        bail)
{
  GeglProperties *o = duster->o;
  int i;
  float score = 0;
  /* bail early with really bad score - the target site doesnt have opacity */

  if (hay[3] < 0.001f)
  {
    return INITIAL_SCORE;
  }

  {
    float sum_x = probe->source_x;
    float sum_y = probe->source_y;
    int count = 1;
    for (int i = 0; i < 4; i++)
    if (neighbors[i])
    {
      sum_x += neighbors[i]->source_x;
      sum_y += neighbors[i]->source_y;
      count++;
    }
    sum_x /= count;
    sum_y /= count;

    score += (POW2(sum_x - probe->source_x) +
             POW2(sum_y - probe->source_y)) * o->metric_cohesion / 1000.0f;
  }

  for (i = 1; i < NEIGHBORHOOD && score < bail; i++)
  {
    float color_diff = 0;
    if (needle[i*4 + 3]<1.0f)
    {
      if (hay[i*4 + 3]>0.001f)
        color_diff = f_rgb_diff (&needle[i*4 + 0], &hay[i*4 + 0]);
      else
        color_diff = o->metric_empty_hay_score;
    }
    else
    {
      color_diff = o->metric_empty_needle_score;
    }
    score += color_diff * duster->order[i][2];
  }

  return score;
}

static Probe *
add_probe (PixelDuster *duster, int target_x, int target_y)
{
  Probe *probe    = g_malloc0 (sizeof (Probe));
  if (target_x < duster->min_x)
    duster->min_x = target_x;
  if (target_y < duster->min_y)
    duster->min_y = target_y;
  if (target_x > duster->max_x)
    duster->max_x = target_x;
  if (target_y > duster->max_y)
    duster->max_y = target_y;

  probe->target_x = target_x;
  probe->target_y = target_y;
  probe->source_x = target_x;
  probe->source_y = target_y;
  probe->score   = INITIAL_SCORE;
  g_hash_table_insert (duster->probes_ht,
                       xy2offset(target_x, target_y), probe);
  return probe;
}

static gfloat *ensure_hay (PixelDuster *duster, int x, int y)
{
#if 1 // when set to 0 - all hay is on demand, and we're ~20% of the speed
  gfloat *hay = NULL;


  if (x < 0 || y < 0 ||
      x > duster->in_rect.width ||
      y > duster->in_rect.height)
  {
    /* alias all requests for hay outside bounding box to same out of
       bounds coords
     */
    x = -100;
    y = -100;
  }

  hay = g_hash_table_lookup (duster->ht[0], xy2offset(x, y));

  if (!hay)
    {
      hay = g_malloc ((4 * NEIGHBORHOOD) * 4);
      extract_site (duster, duster->reference, x, y, 1.0, hay);
      g_hash_table_insert (duster->ht[0], xy2offset(x, y), hay);
    }
#else
  static float hay[4 * NEIGHBORHOOD];
  extract_site (duster, duster->reference, x, y, 1.0, hay);
#endif
  return hay;
}



static float
probe_score (PixelDuster *duster,
             Probe       *probe,
             Probe      **neighbors,
             needles_t    needles,
             int          x,
             int          y,
             gfloat      *hay,
             float        bail);

static inline void
probe_prep (PixelDuster *duster,
            Probe       *probe,
            Probe      **neighbors,
            needles_t    needles)
{
  gint  dst_x  = probe->target_x;
  gint  dst_y  = probe->target_y;
  extract_site (duster, duster->output, dst_x, dst_y, 1.0, &needles[0][0]);
  if (N_SCALE_NEEDLES > 1)
    extract_site (duster, duster->output, dst_x, dst_y, 0.82, &needles[1][0]);
  if (N_SCALE_NEEDLES > 2)
    extract_site (duster, duster->output, dst_x, dst_y, 1.2, &needles[2][0]);
  if (N_SCALE_NEEDLES > 3)
    extract_site (duster, duster->output, dst_x, dst_y, 0.66, &needles[3][0]);
  if (N_SCALE_NEEDLES > 4)
    extract_site (duster, duster->output, dst_x, dst_y, 1.5, &needles[4][0]);
  if (N_SCALE_NEEDLES > 5)
    extract_site (duster, duster->output, dst_x, dst_y, 2.0,&needles[5][0]);
  if (N_SCALE_NEEDLES > 6)
    extract_site (duster, duster->output, dst_x, dst_y, 0.5, &needles[6][0]);

  {
    int coords[16][2]={{-1,0}, // 4 connected
                      {1,0},
                      {0,1},
                      {0,-1},

                      {-1,-1}, // 8 connected
                      {1,1},
                      {-1,1},
                      {1,-1},

                      {-3,0}, // jump of 3 hor/ver
                      {3,0},
                      {0,3},
                      {0,-3},

                      {-8,0}, // jump of 8 hor/ver
                      {8,0},
                      {0,8},
                      {0,-8}
    };
    int neighbours = 0;
    for (int c = 0; c < 16; c++)
    {
       void *off = xy2offset(probe->target_x + coords[c][0],
                             probe->target_y + coords[c][1]);
       Probe *oprobe = g_hash_table_lookup (duster->probes_ht, off);

       if (oprobe)
         neighbors[c] = oprobe;
    }
  }

#ifdef GET_INSPIRED_BY_NEIGHBORS
    for (int i = 0; i < 12; i++)
    {
      if (neighbors[i])
      {
        Probe *oprobe = neighbors[i];
        int coords[8][2]={{-1,0},
                          {1,0},
                          {0,1},
                          {0,-1},

                          {-1,-1},
                          {1,1},
                          {-1,1},
                          {1,-1} 

};
        for (int c = 0; c < 8; c++)
        {
          float test_x = oprobe->source_x + coords[c][0];
          float test_y = oprobe->source_y + coords[c][1];
          float *hay = ensure_hay (duster, test_x, test_y);
          float score = probe_score (duster, probe, neighbors, needles, test_x, test_y, hay, probe->score);
          if (score <= probe->score)
          {
            probe->source_x = test_x;
            probe->source_y = test_y;
            probe->score = score;
          }
        }
      }
    }
#endif
}


static float
probe_score (PixelDuster *duster,
             Probe       *probe,
             Probe      **neighbors,
             needles_t    needles,
             int          x,
             int          y,
             gfloat      *hay,
             float        bail)
{
  float best_score = 10000000.0;
  /* bail early with really bad score - the target site doesnt have opacity */

  if (hay[3] < 0.5f)
  {
    return INITIAL_SCORE;
  }

  for (int n = 0; n < N_SCALE_NEEDLES; n++)
  {
    float score = score_site (duster, probe, neighbors, x, y, &needles[n][0], hay, bail);
    if (score < best_score)
      best_score = score;
  }

  return best_score;
}


static int probe_improve (PixelDuster *duster,
                          Probe       *probe)
{
  Probe *neighbors[16]={NULL,};
  GeglProperties *o = duster->o;
  needles_t needles;
  float old_score = probe->score;

  if (probe->age >= o->rounds)
    {
      g_hash_table_remove (duster->probes_ht,
                           xy2offset(probe->target_x, probe->target_y));
      return -1;
    }

  probe_prep (duster, probe, neighbors, needles);

  {
    float mag = o->seek_distance;
    float startx = probe->source_x;
    float starty = probe->source_y;
    for (int i = 0; i < o->iterations; i++)
    {
      int dx = g_random_int_range (-mag, mag);
      int dy = g_random_int_range (-mag, mag);
      mag *= o->seek_reduction; // reduce seek radius for each iteration
      if (mag < 3)
        mag = 2;
      if (!(dx == 0 && dy == 0))
      {
        int test_x = startx + dx;
        int test_y = starty + dy;

        float *hay = ensure_hay (duster, test_x, test_y);
        float score = probe_score (duster, probe, neighbors, needles, test_x, test_y, hay, probe->score);
        if (score <= probe->score)
        {
          probe->source_x = test_x;
          probe->source_y = test_y;
          probe->score = score;
        }
      }
    }
  }

  probe->age++;

  if (probe->score != old_score)
  {
    gfloat rgba[4];

    gegl_sampler_get (duster->in_sampler_f,
                      probe->source_x, probe->source_y, NULL,
                      &rgba[0], 0);
    rgba[3] = 1.0;
    gegl_buffer_set (duster->output,
                     GEGL_RECTANGLE(probe->target_x, probe->target_y, 1, 1),
                     0, duster->format, &rgba[0], 0);
  }

  return 0;
}

#define MAX_HAY_SIZE     (1024*1024*1024) // 1gb of hay - max

static inline float hay_size_with_overhead (void)
{
  return sizeof (float) * 4 * NEIGHBORHOOD + 32; // guess of hashtable overhead
}

static void
pixel_duster_trim (PixelDuster *duster)
{
  guint     length;
  gint      to_remove = 0;
  gint      max_hays = MAX_HAY_SIZE/hay_size_with_overhead();

  length = g_hash_table_size (duster->ht[0]);
  to_remove = length-max_hays;

  //fprintf (stderr, "%i %i", length, max_hays);

  if (to_remove > 0)
  {
    gpointer *keys;
    keys = g_hash_table_get_keys_as_array (duster->ht[0],
                                           &length);
    while (to_remove)
    {
      int i = g_random_int_range (0, length-1);
      if (keys[i])
      {
        g_hash_table_remove (duster->ht[0], keys[i]);
        to_remove--;
        keys[i] = NULL;
      }
    }
    g_free (keys);
  }


  //malloc_trim (0);

}

static inline void pixel_duster_fill (PixelDuster *duster)
{
  GeglProperties *o = duster->o;
  gint missing = 1;
  gint total = 0;
  gint runs = 0;

  gint max_probes = g_hash_table_size (duster->probes_ht);
  if (max_probes > 10000)
  gegl_operation_progress (duster->op, 0.0, "this may take some time");
  else
  gegl_operation_progress (duster->op, 0.0, "pixel duster");

  while (  (((missing >0) /* && (missing != old_missing) */) ||
           (runs < o->min_iter)) &&
           runs < o->max_iter)
  {
    GList *values;

    runs++;
    total = 0;
    missing = 0;

    values = g_hash_table_get_values (duster->probes_ht);

  for (GList *p= values; p; p= p->next)
  {
    Probe *probe = p->data;
    gint try_replace;

    if (probe->score == INITIAL_SCORE)
    {
      missing ++;
      try_replace =  0;
    }
    else
    {
      try_replace = ((rand()%100)/100.0) < o->chance_retry;
    }
    total ++;

    if (probe->score == INITIAL_SCORE || try_replace)
    {
      if ((rand()%100)/100.0 < o->chance_try)
      {
        probe_improve (duster, probe);
        pixel_duster_trim (duster);
      }
    }
  }

  g_list_free (values);

  //if (duster->op)
  //  gegl_operation_progress (duster->op, (total-missing) * 1.0 / total,
  //                           "finding suitable pixels");
  if (duster->op)
    gegl_operation_progress (duster->op,
       (max_probes - total) * 1.0 /     max_probes,
                             "finding suitable pixels");
#if 1

  fprintf (stderr, "\r%i/%i %2.2f run#:%i  ", total-missing, total,
       (max_probes - total) * 1.0 /     max_probes,
 runs);
#endif
  }
  if (duster->op)
    gegl_operation_progress (duster->op, 1.0, "done");
#if 0
  fprintf (stderr, "\n");
#endif
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;
  return result;
}

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}


static gboolean

process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  GeglRectangle in_rect = *gegl_buffer_get_extent (input);
  GeglRectangle out_rect = *gegl_buffer_get_extent (output);
  PixelDuster    *duster;
  int probes = 0;

  duster  = pixel_duster_new (input, input, output, &in_rect, &out_rect,
                                             o->ring_gap1,
                                             o->ring_gap2,
                                             o->ring_gap3,
                                             o->ring_gap4,
                                             operation);


{
  GeglBufferIterator *i = gegl_buffer_iterator_new (duster->input,
                                                    &duster->out_rect,
                                                    0,
                                                    duster->format,
                                                    GEGL_ACCESS_READ,
                                                    GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (i, duster->output,
                                     &duster->out_rect,
                                     0,
                                     duster->format,
                                     GEGL_ACCESS_WRITE,
                                     GEGL_ABYSS_NONE);
  {
    while (gegl_buffer_iterator_next (i))
    {
      gint x = i->items[0].roi.x;
      gint y = i->items[0].roi.y;
      gint n_pixels  = i->items[0].roi.width * i->items[0].roi.height;
      float *in_pix = i->items[0].data;
      float *out_pix = i->items[1].data;
      while (n_pixels--)
      {
        if (in_pix[3] < 1.0f)
        {
          if (probes ++ < MAX_PROBES)
            add_probe (duster, x, y);
          out_pix[0] = 0.0;
          out_pix[1] = 0.0;
          out_pix[2] = 1.0;
          out_pix[3] = 0.0;
        }
        else
        {
          out_pix[0] = in_pix[0];
          out_pix[1] = in_pix[1];
          out_pix[2] = in_pix[2];
          out_pix[3] = in_pix[3];
        }
        out_pix += 4;
        in_pix += 4;

        x++;
        if (x >= i->items[0].roi.x + i->items[0].roi.width)
        {
          x = i->items[0].roi.x;
          y++;
        }
      }
    }
  }
}

  pixel_duster_fill (duster);

{
  GeglBufferIterator *i = gegl_buffer_iterator_new (duster->input,
                                                    &duster->out_rect,
                                                    0,
                                                    duster->format,
                                                    GEGL_ACCESS_READ,
                                                    GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (i, duster->output,
                                     &duster->out_rect,
                                     0,
                                     duster->format,
                                     GEGL_ACCESS_READWRITE,
                                     GEGL_ABYSS_NONE);
  {
  while (gegl_buffer_iterator_next (i))
  {
    gint x = i->items[0].roi.x;
    gint y = i->items[0].roi.y;
    gint n_pixels  = i->items[0].roi.width * i->items[0].roi.height;
    float *in_pix = i->items[0].data;
    float *out_pix = i->items[1].data;
    while (n_pixels--)
    {
      float alpha = in_pix[3];
      if (in_pix[3] < 1.0f)
      {
        out_pix[0] = (in_pix[0] * alpha) + out_pix[0] * (1.0 - alpha);
        out_pix[1] = (in_pix[1] * alpha) + out_pix[1] * (1.0 - alpha);
        out_pix[2] = (in_pix[2] * alpha) + out_pix[2] * (1.0 - alpha);
        out_pix[3] = out_pix[3];
      }
      out_pix += 4;
      in_pix += 4;

      x++;
      if (x >= i->items[0].roi.x + i->items[0].roi.width)
      {
        x = i->items[0].roi.x;
        y++;
      }
    }
  }
  }
}

  pixel_duster_destroy (duster);
  //if (probes >= MAX_PROBES-1)
  //  goto again;


  return TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if ((in_rect && gegl_rectangle_is_infinite_plane (in_rect)))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->opencl_support          = FALSE;
  operation_class->threaded                = FALSE; // it kind of works, set to TRUE for more performance and possible crashes

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:alpha-inpaint",
      "title",       "Heal transparent",
      "categories",  "heal",
      "description", "Replaces transparent pixels with good candidate pixels found in the neighborhood of the missing pixel.",
      NULL);
}

#endif 
