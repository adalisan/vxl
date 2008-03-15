#include <testlib/testlib_test.h>
#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vul/vul_file.h>

#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_box_3d.h>
#include <vgl/vgl_point_3d.h>

#include "../bvxm_voxel_grid.h"
#include "../bvxm_voxel_slab.h"
#include "../bvxm_voxel_world.h"
#include "../bvxm_world_params.h"
#include "../bvxm_mog_grey_processor.h"

#include <vpgl/vpgl_perspective_camera.h>
#include <vpgl/vpgl_rational_camera.h>
#include <vpgl/vpgl_calibration_matrix.h>

int IMAGE_U = 250;
int IMAGE_V = 250;
double x_scale = 700;
double y_scale = 700;
double focal_length = 1.;
double camera_dist= 200;

//size of the world
unsigned nx=100, ny=100, nz=50;
float vox_length = 1.0f;

typedef bvxm_voxel_traits<APM_MOG_GREY>::voxel_datatype apm_datatype;

// returns a face number if a point is on the surface of a box [0,1,2,3,4,5],
// -1 otherwise (not on the surface case)
int on_box_surface(vgl_box_3d<double> box, vgl_point_3d<double> v)
{
  // create a box a size smaller
  vgl_box_3d<double> in_box;
  int thickness = 1;
  in_box.set_min_point(vgl_point_3d<double>(box.min_x()+thickness, box.min_y()+thickness, box.min_z()+thickness));
  in_box.set_max_point(vgl_point_3d<double>(box.max_x()-thickness, box.max_y()-thickness, box.max_z()-thickness));

  bool on = false;
  if (box.contains(v) && !in_box.contains(v)) {
    on = true;

    // find the face index
    if ((box.min_z() <= v.z()) && (v.z() <= box.min_z()+thickness))
      return 0;
    
    if ((box.max_z()-thickness <= v.z()) && (v.z() <= box.max_z()))
      return 5;

    if ((box.min_y() <= v.y()) && (v.y() <= box.min_y()+thickness))
      return 1;

    if ((box.max_y()-thickness <= v.y()) && (v.y() <= box.max_y()))
      return 3;

    if ((box.min_x() <= v.x()) && (v.x() <= box.min_x()+thickness))
      return 4;

    if ((box.max_x()-thickness <= v.x()) && (v.x() <= box.max_x()))
      return 2;
  }
  return -1;
}

int in_box(vgl_box_3d<double> box, vgl_point_3d<double> v)
{
  if (box.contains(v))
    return 1;
  else 
    return -1;
}

void create_top_box(vgl_box_3d<double> box, vgl_box_3d<double>& top_box, 
                    double dimx, double dimy, double dimz)
{
  // find top face center
  vgl_point_3d<double> centroid = box.centroid();
  centroid.set(centroid.x(), centroid.y(), box.max_z() + dimz/2.0);
  top_box = vgl_box_3d<double> (centroid, dimx, dimy, dimz, vgl_box_3d<double>::centre);
  // translate it a bit
  vgl_point_3d<double> top_centroid = top_box.centroid();
  top_box.set_centroid(vgl_point_3d<double>(top_centroid.x()+dimx/3., top_centroid.y()+dimx/3., top_centroid.z()));
  vcl_cout << top_box << vcl_endl;
}

vcl_vector<vgl_point_3d<double> > 
corners_of_box_3d(vgl_box_3d<double> box)
{
  vcl_vector<vgl_point_3d<double> > corners;

  corners.push_back(box.min_point());
  corners.push_back(vgl_point_3d<double> (box.min_x()+box.width(), box.min_y(), box.min_z()));
  corners.push_back(vgl_point_3d<double> (box.min_x()+box.width(), box.min_y()+box.height(), box.min_z()));
  corners.push_back(vgl_point_3d<double> (box.min_x(), box.min_y()+box.height(), box.min_z()));
  corners.push_back(vgl_point_3d<double> (box.min_x(), box.min_y(), box.max_z()));
  corners.push_back(vgl_point_3d<double> (box.min_x()+box.width(), box.min_y(), box.max_z()));
  corners.push_back(box.max_point());
  corners.push_back(vgl_point_3d<double> (box.min_x(), box.min_y()+box.height(), box.max_z()));
  return corners;
}

void generate_persp_camera(double focal_length,
                           vgl_point_2d<double>& pp,  //principal point
                           double x_scale, double y_scale,
                           vgl_point_3d<double>& camera_center,
                           vpgl_perspective_camera<double>& cam)
{
  vpgl_calibration_matrix<double> K(focal_length,pp, x_scale, y_scale);
  cam.set_calibration(K);
  cam.set_camera_center(camera_center);
}

vpgl_rational_camera<double>
perspective_to_rational(vpgl_perspective_camera<double>& cam_pers)
{
  vnl_matrix_fixed<double,3,4> cam_pers_matrix = cam_pers.get_matrix();
  vcl_vector<double> neu_u,den_u,neu_v,den_v;
  double x_scale = 1.0,
         x_off = 0.0,
         y_scale = 1.0,
         y_off = 0.0,
         z_scale = 1.0,
         z_off = 0.0,
         u_scale = 1.0,
         u_off = 0.0,
         v_scale = 1.0,
         v_off = 0.0;

  for(int i=0; i<20; i++){
    neu_u.push_back(0.0);
    neu_v.push_back(0.0);
    den_u.push_back(0.0);
    den_v.push_back(0.0);
  }

  int vector_map[] = {9,15,18,19};

  for(int i=0; i<4; i++){
    neu_u[vector_map[i]] = cam_pers_matrix(0,i);
    neu_v[vector_map[i]] = cam_pers_matrix(1,i);
    den_u[vector_map[i]] = cam_pers_matrix(2,i);
    den_v[vector_map[i]] = cam_pers_matrix(2,i);
  }

  vpgl_rational_camera<double> cam_rat(neu_u,den_u,neu_v,den_v,
    x_scale,x_off,y_scale,y_off,z_scale,z_off,u_scale,u_off,v_scale,v_off);
  return cam_rat;
}


#if 0
vcl_vector<vpgl_camera_double_sptr >
generate_cameras_z(vgl_box_3d<double>& world)
{
  vgl_point_2d<double> principal_point(IMAGE_U/2., IMAGE_V/2.);

  vgl_point_3d<double> centroid = world.centroid();
  double x,z;
  double alpha = (vnl_math::pi/8.) * 3;
  double delta_alpha = vnl_math::pi/40.;
  vcl_vector<vgl_point_3d<double> > centers;
  for (unsigned i=0; i<11; i++) {
    x = camera_dist*cos(alpha);
    z = camera_dist*sin(alpha);
    centers.push_back(vgl_point_3d<double> (centroid.x()+x, centroid.y(), centroid.z()+z));
    vcl_cout << centers[i] << vcl_endl;
    alpha += delta_alpha;
  }

  vgl_box_2d<double> bb;
  vcl_vector<vpgl_camera_double_sptr> rat_cameras;
  for(unsigned i=0; i<centers.size(); i++) {
    vgl_point_3d<double> camera_center  = centers[i];
    vpgl_perspective_camera<double> persp_cam;
    generate_persp_camera(focal_length,principal_point, x_scale, y_scale, camera_center, persp_cam);
    persp_cam.look_at(vgl_homg_point_3d<double>(centroid));
    vpgl_rational_camera<double>* rat_cam = new vpgl_rational_camera<double>(perspective_to_rational(persp_cam));
    rat_cameras.push_back(rat_cam);

    vcl_vector<vgl_point_3d<double> > corners = corners_of_box_3d(world);
    for(unsigned i=0; i<corners.size(); i++) {
      vgl_point_3d<double> c = corners[i];
      double u,v, u2, v2;
      persp_cam.project(c.x(), c.y() ,c.z(), u, v);
      rat_cam->project(c.x(), c.y() ,c.z(), u2, v2);
      bb.add(vgl_point_2d<double> (u,v));
      //vcl_cout << "Perspective [" << u << "," << v << "]" << vcl_endl;
      //vcl_cout << "Rational [" << u2 << "," << v2 << "]" << vcl_endl << vcl_endl;
    }
    vcl_cout << bb << vcl_endl;
  }
  return rat_cameras;
}
#endif 

vcl_vector<vpgl_camera_double_sptr >
generate_cameras_yz(vgl_box_3d<double>& world)
{
  vgl_point_2d<double> principal_point(IMAGE_U/2., IMAGE_V/2.);

  vgl_point_3d<double> centroid = world.centroid();
  double x,y;
  double alpha = (vnl_math::pi/8.) * 3;
  double delta_alpha = vnl_math::pi/40.;
  vcl_vector<vgl_point_3d<double> > centers;
  for (unsigned i=0; i<11; i++) {
    x = camera_dist*cos(alpha);
    y = camera_dist*sin(alpha);
    centers.push_back(vgl_point_3d<double> (x+centroid.x(), y+centroid.y(), 450+centroid.z()));
    vcl_cout << centers[i] << vcl_endl;
    alpha += delta_alpha;
  }

  vgl_box_2d<double> bb;
  vcl_vector<vpgl_camera_double_sptr> rat_cameras;
  for(unsigned i=0; i<centers.size(); i++) {
    vgl_point_3d<double> camera_center  = centers[i];
    vpgl_perspective_camera<double> persp_cam;
    generate_persp_camera(focal_length,principal_point, x_scale, y_scale, camera_center, persp_cam);
    persp_cam.look_at(vgl_homg_point_3d<double>(centroid));
    vpgl_rational_camera<double>* rat_cam = new vpgl_rational_camera<double>(perspective_to_rational(persp_cam));
    rat_cameras.push_back(rat_cam);

    vcl_vector<vgl_point_3d<double> > corners = corners_of_box_3d(world);
    for(unsigned i=0; i<corners.size(); i++) {
      vgl_point_3d<double> c = corners[i];
      double u,v, u2, v2;
      persp_cam.project(c.x(), c.y() ,c.z(), u, v);
      rat_cam->project(c.x(), c.y() ,c.z(), u2, v2);
      bb.add(vgl_point_2d<double> (u,v));
      //vcl_cout << "Perspective [" << u << "," << v << "]" << vcl_endl;
      //vcl_cout << "Rational [" << u2 << "," << v2 << "]" << vcl_endl << vcl_endl;
    }
    vcl_cout << bb << vcl_endl;
  }
  return rat_cameras;
}

bool gen_images(vgl_vector_3d<unsigned> grid_size,
                bvxm_voxel_world_sptr world,
               // bvxm_world_params_sptr world_params,
                bvxm_voxel_grid<float>* intensity_grid,
                bvxm_voxel_grid<float>* ocp_grid,
                bvxm_voxel_grid<apm_datatype>* apm_grid,
                vcl_vector<vpgl_camera_double_sptr>& cameras, 
                vcl_vector <vil_image_view_base_sptr>& image_set,
                unsigned int bin_num)
{
  
  apm_datatype sample;
  apm_grid->initialize_data(sample);

  bvxm_voxel_grid<apm_datatype>::iterator apm_slab_it;
  bvxm_voxel_grid<float>::iterator obs_it = intensity_grid->begin();
  bool update_status;
  //slab thickness is 1 
  bvxm_voxel_slab<float>* weight = new bvxm_voxel_slab<float>(grid_size.x(),grid_size.y(),1);  
  weight->fill(1);

  for (apm_slab_it = apm_grid->begin(); apm_slab_it != apm_grid->end(); ++apm_slab_it,++obs_it) {
    bvxm_mog_grey_processor processor;
    update_status = processor.update(*apm_slab_it, *obs_it, *weight);
  }

  vcl_string path = "./test_gen_synthetic_world/test_img";
  for (unsigned i=0; i<cameras.size(); i++) {
    vil_image_view_base_sptr img_arg;
    vil_image_view<float>* mask = new vil_image_view<float>(IMAGE_U, IMAGE_V);
    vil_image_view_base_sptr expected = new vil_image_view<unsigned char>(IMAGE_U, IMAGE_V);
    bvxm_image_metadata camera(img_arg, cameras[i]);

    world->expected_image<APM_MOG_GREY>(camera, expected, *mask, bin_num);
    vcl_stringstream s;
    s << path << i << "_" << bin_num << ".tif";
    vil_save(*expected, s.str().c_str());
    image_set.push_back(expected);
  }
  return true;

}

bool reconstruct_world( bvxm_voxel_world_sptr recon_world, vcl_vector<vpgl_camera_double_sptr>& cameras, 
                 vcl_vector <vil_image_view_base_sptr>& image_set,unsigned int bin_num)
{
  vcl_string recon_path = "recon_world/test_img";
  vcl_string camera_path = "test_gen_cameras/camera";

  for (unsigned i = 0;i<cameras.size();i++)
  {
    bvxm_image_metadata observation(image_set[i],cameras[i]);
    recon_world->update<APM_MOG_GREY>(observation,bin_num);
  }

   for (unsigned i=0; i<cameras.size(); i++) {
    vil_image_view_base_sptr img_arg;
    vil_image_view<float> mask(IMAGE_U, IMAGE_V);
    vil_image_view_base_sptr expected = new vil_image_view<unsigned char>(IMAGE_U, IMAGE_V);
    bvxm_image_metadata camera(img_arg, cameras[i]);


    recon_world->expected_image<APM_MOG_GREY>(camera, expected, mask);
    vcl_stringstream s;
    s << recon_path << i << "_" << bin_num << ".tif";
    vcl_stringstream c;
    c << camera_path << i << ".rpc";
    vil_save(*expected, s.str().c_str());
    vpgl_rational_camera<double>* cam = static_cast<vpgl_rational_camera<double>*> (cameras[i].as_pointer());
    cam->save(c.str().c_str());
    image_set.push_back(expected);
  }
   return true;
}

void gen_voxel_world_2box(vgl_vector_3d<unsigned> grid_size,
                          vgl_box_3d<double> voxel_world,
                          bvxm_voxel_grid<float>* ocp_grid,
                          bvxm_voxel_grid<float>* intensity_grid)
{
  
  // fill with test data
  float init_val = 0.01;
  ocp_grid->initialize_data(init_val);
  intensity_grid->initialize_data(init_val);

  bvxm_voxel_grid<float>::iterator ocp_slab_it;
  bvxm_voxel_grid<float>::iterator intensity_slab_it = intensity_grid->begin();

  //object (essentially a box) placed in the voxel world
  vgl_box_3d<double> box(vgl_point_3d<double> (10,10,10), 
    vgl_point_3d<double> (80,80,30));
 
  vgl_box_3d<double> top_box;
  create_top_box(box, top_box, 30,30,10);

  float face_intens[6] = {0.3, 0.45, 0.6, 0.75, 0.85, 1.0};
  vcl_ofstream is("test_gen_synthetic_world/intensity_grid.txt");
  unsigned z=nz;
  for (ocp_slab_it = ocp_grid->begin(); 
    ocp_slab_it != ocp_grid->end(); 
    ++ocp_slab_it,++intensity_slab_it) {
    z--;
    is << z << "--->" << vcl_endl;
    bvxm_voxel_slab<float>::iterator vit,iit;
    
    iit = intensity_slab_it->begin();
    vit = ocp_slab_it->begin();
    for (unsigned i=0; i<nx; i++) {
      is << vcl_endl;
      for (unsigned j=0; j<ny; j++) {
        int face1 = on_box_surface(box, vgl_point_3d<double>(i,j,z));
        int face2 = on_box_surface(top_box, vgl_point_3d<double>(i,j,z));
        if ((face1 != -1) || (face2 != -1)){
          if (face1 != -1)
            (*intensity_slab_it)(i,j,0) = face_intens[face1];
          else 
            (*intensity_slab_it)(i,j,0) = face_intens[5-face2];
          (*ocp_slab_it)(i,j,0) = 1.0f;
          is << " x" ;
        } else
          is << " 0";
      }
    }
  }
  vcl_cout << "grid done." << vcl_endl;
}

void gen_voxel_world_plane(vgl_vector_3d<unsigned> grid_size,
                           vgl_box_3d<double> voxel_world,
                           bvxm_voxel_grid<float>* ocp_grid,
                           bvxm_voxel_grid<float>* intensity_grid)
{
  
  // fill with test data
  float init_val = 0.0;
  ocp_grid->initialize_data(init_val);
  intensity_grid->initialize_data(init_val);

  bvxm_voxel_grid<float>::iterator ocp_slab_it;
  bvxm_voxel_grid<float>::iterator intensity_slab_it = intensity_grid->begin();

  vgl_box_3d<double> plane_box(vgl_point_3d<double> (20,20,24), 
    vgl_point_3d<double> (80, 80, 25));

  vcl_ofstream is("test_gen_synthetic_world/intensity_grid.txt");
  unsigned z=nz;
  for (ocp_slab_it = ocp_grid->begin(); 
    ocp_slab_it != ocp_grid->end(); 
    ++ocp_slab_it,++intensity_slab_it) {
    z--;
    is << z << "--->" << vcl_endl;
    bvxm_voxel_slab<float>::iterator vit,iit;

    iit = intensity_slab_it->begin();
    vit = ocp_slab_it->begin();
    for (unsigned i=0; i<nx; i++) {
      is << vcl_endl;
      for (unsigned j=0; j<ny; j++) {
        int face1 = on_box_surface(plane_box, vgl_point_3d<double>(i,j,z));
        if (face1 != -1) {  
          (*intensity_slab_it)(i,j,0) = 0.8;
          (*ocp_slab_it)(i,j,0) = 1.0f;
          is << " x" ;
        } else
          is << " 0";
      }
    }
  }
  vcl_cout << "grid done." << vcl_endl;
}

static void test_gen_synthetic_world()
{
  START("test_gen_synthetic_world test");

  // create the directory under build to put the intermediate files and the generated images
  vcl_string model_dir("./test_gen_synthetic_world");
  vul_file::make_directory(model_dir);

  vcl_string recon_model_dir("./recon_world");
  vul_file::make_directory(recon_model_dir);

  vul_file::make_directory("./test_gen_cameras");

  vgl_vector_3d<unsigned> grid_size(nx,ny,nz);
  vgl_box_3d<double> voxel_world(vgl_point_3d<double> (0,0,0), 
    vgl_point_3d<double> (nx, ny, nz));

  bvxm_world_params_sptr world_params = new bvxm_world_params();
  world_params->set_params("./test_gen_synthetic_world",
    vgl_point_3d<float> (0,0,0),
    vgl_vector_3d<unsigned int>(nx, ny, nz), 
    vox_length);
  bvxm_voxel_world_sptr world = new bvxm_voxel_world();
  world->set_params(world_params);
  world->clean_grids();

  unsigned int bin_num_1 = 0,bin_num_2 = 10;
  //create an mog grid for appearance model and use appearence model processor update to properly initialize it
  bvxm_voxel_grid<float>* ocp_grid = static_cast<bvxm_voxel_grid<float>* > 
    (world->get_grid<OCCUPANCY>(0).as_pointer());

  bvxm_voxel_grid<apm_datatype>* apm_grid_1 = static_cast<bvxm_voxel_grid<apm_datatype>* >
    (world->get_grid<APM_MOG_GREY>(bin_num_1).as_pointer());

    bvxm_voxel_grid<apm_datatype>* apm_grid_2 = static_cast<bvxm_voxel_grid<apm_datatype>* >
    (world->get_grid<APM_MOG_GREY>(bin_num_2).as_pointer());

  bvxm_voxel_grid<float>* intensity_grid = new bvxm_voxel_grid<float>
    ("test_gen_synthetic_world/intensity.vox",grid_size);

  gen_voxel_world_2box(grid_size, voxel_world, ocp_grid, intensity_grid);
  vcl_vector<vpgl_camera_double_sptr> cameras = generate_cameras_yz(voxel_world);

  vcl_vector <vil_image_view_base_sptr> image_set_1,image_set_2;

  // generate images from synthetis world
  gen_images(grid_size, world, intensity_grid, ocp_grid, apm_grid_1, 
    cameras, image_set_1, bin_num_1);

  gen_images(grid_size, world, intensity_grid, ocp_grid, apm_grid_2, 
    cameras, image_set_2, bin_num_2);
  //reconstruct the world from synthehtic imag  

  world->save_occupancy_raw("./test_gen_synthetic_world/ocp.raw");

  bvxm_voxel_world_sptr recon_world = new bvxm_voxel_world();

  bvxm_world_params_sptr recon_world_params = new bvxm_world_params();
  recon_world_params->set_params("./recon_world",vgl_point_3d<float> (0,0,0),
    vgl_vector_3d<unsigned int>(nx, ny, nz), vox_length);

  recon_world->set_params(recon_world_params);
  recon_world->clean_grids();
  recon_world->get_grid<APM_MOG_GREY>(bin_num_1);
  recon_world->get_grid<APM_MOG_GREY>(bin_num_2);
  recon_world->get_grid<OCCUPANCY>(0);

  //reconstruct the world from synthetic images and the apm grid stored in bin bin_num_1
  reconstruct_world(recon_world,cameras, image_set_1,bin_num_1);
  recon_world->save_occupancy_raw("./recon_world/ocp1.raw");

  reconstruct_world(recon_world,cameras, image_set_2,bin_num_2);
  recon_world->save_occupancy_raw("./recon_world/ocp2.raw");

  return;
}

TESTMAIN( test_gen_synthetic_world );


