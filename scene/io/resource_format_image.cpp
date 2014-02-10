/*************************************************************************/
/*  resource_format_image.cpp                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "resource_format_image.h"
#include "scene/resources/texture.h"
#include "io/image_loader.h"
#include "globals.h"
#include "os/os.h"
RES ResourceFormatLoaderImage::load(const String &p_path,const String& p_original_path) {
	
	
	if (p_path.extension()=="cube") {
		// open as cubemap txture

		CubeMap* ptr = memnew(CubeMap);
		Ref<CubeMap> cubemap( ptr );

		Error err;
		FileAccess *f = FileAccess::open(p_path,FileAccess::READ,&err);
		if (err) {
		
			ERR_FAIL_COND_V( err, RES() );
		}
		
		String base_path=p_path.substr( 0, p_path.find_last("/")+1 );

		for(int i=0;i<6;i++) {
		
			String file = f->get_line().strip_edges();
			Image image;
			
			Error err = ImageLoader::load_image(base_path+file,&image);

			
			if (err) {
			
				memdelete(f);
				ERR_FAIL_COND_V( err, RES() );
			}
			
			if (i==0) {
			
				//cubemap->create(image.get_width(),image.get_height(),image.get_format(),Texture::FLAGS_DEFAULT|Texture::FLAG_CUBEMAP);
			}
			
			static const CubeMap::Side cube_side[6]= {
				CubeMap::SIDE_LEFT,
				CubeMap::SIDE_RIGHT,
				CubeMap::SIDE_BOTTOM,
				CubeMap::SIDE_TOP,
				CubeMap::SIDE_FRONT,
				CubeMap::SIDE_BACK
			};
			
			cubemap->set_side(cube_side[i],image);
		}
		
		memdelete(f);

		cubemap->set_name(p_path.get_file());

		return cubemap;
	
	} else {
		// simple image	

		ImageTexture* ptr = memnew(ImageTexture);
		Ref<ImageTexture> texture( ptr );

		uint64_t begtime;
		double total;

		Image image;

		if (debug_load_times)
			begtime=OS::get_singleton()->get_ticks_usec();


		Error err = ImageLoader::load_image(p_path,&image);

		if (!err && debug_load_times) {
			double total=(double)(OS::get_singleton()->get_ticks_usec()-begtime)/1000000.0;
			print_line("IMAGE: "+itos(image.get_width())+"x"+itos(image.get_height()));
			print_line("  -load: "+rtos(total));
		}


		ERR_EXPLAIN("Failed loading image: "+p_path);
		ERR_FAIL_COND_V(err, RES());		

#ifdef DEBUG_ENABLED
#ifdef TOOLS_ENABLED

		if (max_texture_size && (image.get_width() > max_texture_size || image.get_height() > max_texture_size)) {


			if (bool(Globals::get_singleton()->get("debug/max_texture_size_alert"))) {
				OS::get_singleton()->alert("Texture is too large: '"+p_path+"', at "+itos(image.get_width())+"x"+itos(image.get_height())+". Max allowed size is: "+itos(max_texture_size)+"x"+itos(max_texture_size)+".","BAD ARTIST, NO COOKIE!");
			}

			ERR_EXPLAIN("Texture is too large: '"+p_path+"', at "+itos(image.get_width())+"x"+itos(image.get_height())+". Max allowed size is: "+itos(max_texture_size)+"x"+itos(max_texture_size)+".");
			ERR_FAIL_V(RES());
		}
#endif
#endif
		
		
		uint32_t flags=0;
		if (bool(GLOBAL_DEF("texture_import/filter",true)))
			flags|=Texture::FLAG_FILTER;
		if (bool(GLOBAL_DEF("texture_import/gen_mipmaps",true)))
			flags|=Texture::FLAG_MIPMAPS;
		if (bool(GLOBAL_DEF("texture_import/repeat",true)))
			flags|=Texture::FLAG_REPEAT;



		if (debug_load_times)
			begtime=OS::get_singleton()->get_ticks_usec();

		//print_line("img: "+p_path+" flags: "+itos(flags));
		texture->create_from_image( image,flags );
		texture->set_name(p_path.get_file());


		if (debug_load_times) {
			total=(double)(OS::get_singleton()->get_ticks_usec()-begtime)/1000000.0;
			print_line("  -make texture: "+rtos(total));
		}

		return RES( texture );
	}
	

}

bool ResourceFormatLoaderImage::handles_type(const String& p_type) const {

	return ObjectTypeDB::is_type(p_type,"Texture") || ObjectTypeDB::is_type(p_type,"CubeMap");
}

void ResourceFormatLoaderImage::get_recognized_extensions(List<String> *p_extensions) const {
	
	ImageLoader::get_recognized_extensions(p_extensions);
	p_extensions->push_back("cube");
}

String ResourceFormatLoaderImage::get_resource_type(const String &p_path) const {

	String ext=p_path.extension().to_lower();
	if (ext=="cube")
		return "CubeMap";

	List<String> extensions;
	ImageLoader::get_recognized_extensions(&extensions);

	for(List<String>::Element *E=extensions.front();E;E=E->next()) {
		if (E->get()==ext)
			return "ImageTexture";
	}
	return "";
}


ResourceFormatLoaderImage::ResourceFormatLoaderImage() {

	max_texture_size = GLOBAL_DEF("debug/max_texture_size",0);
	GLOBAL_DEF("debug/max_texture_size_alert",false);
	debug_load_times=GLOBAL_DEF("debug/image_load_times",false);
	GLOBAL_DEF("texture_import/filter",true);
	GLOBAL_DEF("texture_import/gen_mipmaps",true);
	GLOBAL_DEF("texture_import/repeat",true);

}