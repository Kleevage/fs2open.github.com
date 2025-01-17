/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 



#include "anim/animplay.h"
#include "anim/packunpack.h"
#include "bmpman/bmpman.h"
#include "graphics/2d.h"


const int packer_code = PACKER_CODE;
const int transparent_code = 254;

anim_instance *init_anim_instance(anim *ptr, int bpp)
{
	anim_instance *inst;

	if (!ptr) {
		Int3();
		return NULL;
	}

	if ( ptr->flags & ANF_STREAMED ) {
		if ( ptr->file_offset < 0 ) {
			Int3();
			return NULL;
		}
	} else {
		if ( !ptr->data ) {
			Int3();
			return NULL;
		}
	}

	ptr->instance_count++;
	inst = (anim_instance *) vm_malloc(sizeof(anim_instance));
	Assert(inst);
	memset(inst, 0, sizeof(anim_instance));
	inst->frame_num = -1;
	inst->last_frame_num = -1;
	inst->parent = ptr;
	inst->data = ptr->data;
	inst->file_offset = ptr->file_offset;
	inst->stop_now = FALSE;
	inst->aa_color = NULL;

	inst->frame = (ubyte *) vm_malloc(inst->parent->width * inst->parent->height * (bpp >> 3));
	Assert( inst->frame != NULL );
	memset( inst->frame, 0, inst->parent->width * inst->parent->height * (bpp >> 3) );

	return inst;
}

void free_anim_instance(anim_instance *inst)
{
	Assert(inst->frame);
	vm_free(inst->frame);
	inst->frame = NULL;
	inst->parent->instance_count--;	
	inst->parent = NULL;
	inst->data = NULL;
	inst->file_offset = -1;

	vm_free(inst);	
}

ubyte *anim_get_next_raw_buffer(anim_instance *inst, int xlate_pal, int aabitmap, int bpp)
{	
	if ( anim_instance_is_streamed(inst) ) {
		if ( inst->file_offset < 0 ) {
			return NULL;
		}
	} else {
		if (!inst->data){
			return NULL;
		}
	}

	inst->frame_num++;
	if (inst->frame_num >= inst->parent->total_frames) {
		inst->data = NULL;
		inst->file_offset = inst->parent->file_offset;
		return NULL;
	}

	if ( anim_instance_is_streamed(inst) ) {
		if ( xlate_pal ){
			inst->file_offset = unpack_frame_from_file(inst, inst->frame, inst->parent->width*inst->parent->height, inst->parent->palette_translation, aabitmap, bpp);
		} else {
			inst->file_offset = unpack_frame_from_file(inst, inst->frame, inst->parent->width*inst->parent->height, NULL, aabitmap, bpp);
		}
	} else {
		if ( xlate_pal ){
			inst->data = unpack_frame(inst, inst->data, inst->frame, inst->parent->width*inst->parent->height, inst->parent->palette_translation, aabitmap, bpp);
		} else {
			inst->data = unpack_frame(inst, inst->data, inst->frame, inst->parent->width*inst->parent->height, NULL, aabitmap, bpp);
		}
	}

	return inst->frame;
}

/**
 * @brief Convert a 24 bit value to a 16 bit value
 * @param bit_24 24 bit value
 * @param bit_16 16 bit value (output)
 */
void convert_24_to_16(int bit_24, ushort *bit_16)
{
	ubyte *pixel = (ubyte*)&bit_24;
	ubyte alpha = 1;

	bm_set_components((ubyte*)bit_16, (ubyte*)&pixel[0], (ubyte*)&pixel[1], (ubyte*)&pixel[2], &alpha);
}

/**
 * @brief Unpack a pixel given the passed index and the anim_instance's palette
 * @return Bytes stuffed
 */
int unpack_pixel(anim_instance *ai, ubyte *data, ubyte pix, int aabitmap, int bpp)
{
	int bit_24;
	ushort bit_16 = 0;	
	ubyte bit_8 = 0;
	ubyte al = 0;
	ubyte r, g, b;
	int pixel_size = (bpp / 8);
	anim *a = ai->parent;
	Assert(a);

	// if this is an aabitmap, don't run through the palette
	if(aabitmap){
		switch(bpp){
		case 16 : 
			bit_16 = (ushort)pix;
			break;
		case 8:
			// 8 bit-per-pixel aa bitmaps are a bit special since they only use a palette index value in the range [0, 15]. These 
			// palette indexes must be remapped to alpha values between [0, 255] which is what graphics code expects. Palette 
			// range [0, 14] is a gradient from black to white, and palette index 15 is a special color which indicates the background
			// area of a HUD gauge. Retail code uses the final alpha value for index 1 for this special index to give gauges a dark
			// transparent background.
			if (pix > 15) {
				bit_8 = 255;
			}
			else if (pix == 15) {
				bit_8 = 18;
			}
			else {
				bit_8 = (ubyte)(pix * 18);
			}

			break;
		default:
			Int3();
		}
	} else {		
		// if the pixel value is 255, or is the xparent color, make it so		
		if(((a->palette[pix*3] == a->xparent_r) && (a->palette[pix*3+1] == a->xparent_g) && (a->palette[pix*3+2] == a->xparent_b)) ){
			if (pixel_size > 2) {
				bit_24 = 0;
			} else {
				r = b = 0;
				g = 255;

				bm_set_components((ubyte*)&bit_16, &r, &g, &b, &al);
			}
		} else {
			if (pixel_size > 2) {
				ubyte pixel[4];
				pixel[0] = ai->parent->palette[pix * 3 + 2];
				pixel[1] = ai->parent->palette[pix * 3 + 1];
				pixel[2] = ai->parent->palette[pix * 3];
				pixel[3] = 255;
				memcpy(&bit_24, pixel, sizeof(int));

				if (pixel_size == 4) {
					bit_24 = INTEL_INT(bit_24);
				}
			} else {
				// stuff the 24 bit value
				memcpy(&bit_24, &ai->parent->palette[pix * 3], 3); //-V512

				// convert to 16 bit
				convert_24_to_16(bit_24, &bit_16);
			}
		}
	}

	// stuff the pixel
	switch (bpp) {
		case 32:
		case 24:
			memcpy(data, &bit_24, pixel_size);
			break;

		case 16:
			memcpy(data, &bit_16, pixel_size);
			break;

		case 8:
			*data = bit_8;
			break;

		default:
			Int3();
			return 0;
	}

	return pixel_size;
}

/**
 * @brief Unpack a pixel given the passed index and the anim_instance's palette
 * @return Bytes stuffed
 */
int unpack_pixel_count(anim_instance *ai, ubyte *data, ubyte pix, int count = 0, int aabitmap = 0, int bpp = 8)
{
	int bit_24;
	int idx;
	ubyte al = 0;
	ushort bit_16 = 0;	
	ubyte bit_8 = 0;
	anim *a = ai->parent;
	int pixel_size = (bpp / 8);
	ubyte r, g, b;
	Assert(a);	

	// if this is an aabitmap, don't run through the palette
	if(aabitmap){
		switch(bpp){
		case 16 : 
			bit_16 = (ushort)pix;
			break;
		case 8 :
			// 8 bit-per-pixel aa bitmaps are a bit special since they only use a palette index value in the range [0, 15]. These 
			// palette indexes must be remapped to alpha values between [0, 255] which is what graphics code expects. Palette 
			// range [0, 14] is a gradient from black to white, and palette index 15 is a special color which indicates the background
			// area of a HUD gauge. Retail code uses the final alpha value for index 1 for this special index to give gauges a dark
			// transparent background.
			if (pix > 15) {
				bit_8 = 255;
			}
			else if (pix == 15) {
				bit_8 = 18;
			}
			else {
				bit_8 = (ubyte)(pix * 18);
			}

			break;
		default :
			Int3();			
		}
	} else {		
		// if the pixel value is 255, or is the xparent color, make it so		
		if(((a->palette[pix*3] == a->xparent_r) && (a->palette[pix*3+1] == a->xparent_g) && (a->palette[pix*3+2] == a->xparent_b)) ){
			if (pixel_size > 2) {
				bit_24 = 0;
			} else {
				r = b = 0;
				g = 255;
				
				bm_set_components((ubyte*)&bit_16, &r, &g, &b, &al);
			}
		} else {
			if (pixel_size > 2) {
				ubyte pixel[4];
				pixel[0] = ai->parent->palette[pix * 3 + 2];
				pixel[1] = ai->parent->palette[pix * 3 + 1];
				pixel[2] = ai->parent->palette[pix * 3];
				pixel[3] = 255;
				memcpy(&bit_24, pixel, sizeof(int));

				if (pixel_size == 4) {
					bit_24 = INTEL_INT(bit_24);
				}
			} else {
				// stuff the 24 bit value
				memcpy(&bit_24, &ai->parent->palette[pix * 3], 3); //-V512
				
				// convert to 16 bit
				convert_24_to_16(bit_24, &bit_16);
			}
		}
	}
	
	// stuff the pixel
	for (idx=0; idx<count; idx++) {
		switch (bpp) {
			case 32:
			case 24:
				memcpy(data + (idx * pixel_size), &bit_24, pixel_size);
				break;

			case 16:
				memcpy(data + (idx * pixel_size), &bit_16, pixel_size);
				break;

			case 8:
				*(data + idx) = bit_8;
				break;
			}
	}

	return (pixel_size * count);
}

/**
 * @brief Unpack frame
 *
 * @param ai Animation instance
 * @param ptr Packed data to unpack
 * @param frame Where to store unpacked data to
 * @param size Total number of unpacked pixels requested
 * @param pal_translate Color translation lookup table (NULL if no palette translation desired)
 * @param aabitmap
 * @param bpp
 */
ubyte	*unpack_frame(anim_instance *ai, ubyte *ptr, ubyte *frame, int size, ubyte *pal_translate, int aabitmap, int bpp)
{
	int	xlate_pal, value, count = 0;
	int stuffed;			
	int pixel_size = (bpp / 8);

	if ( pal_translate == NULL ) {
		xlate_pal = 0;
	}
	else {
		xlate_pal = 1;
	}

	if (*ptr == PACKING_METHOD_RLE_KEY) {  // key frame, Hoffoss's RLE format
		ptr++;
		while (size > 0) {
			value = *ptr++;
			if (value != packer_code) {
				if ( xlate_pal ){
					stuffed = unpack_pixel(ai, frame, pal_translate[value], aabitmap, bpp);
				} else {
					stuffed = unpack_pixel(ai, frame, (ubyte)value, aabitmap, bpp);
				}
				frame += stuffed;
				size--;
			} else {
				count = *ptr++;
				if (count < 2){
					value = packer_code;
				} else {
					value = *ptr++;
				}

				if (++count > size){
					count = size;
				}

				if ( xlate_pal ){
					stuffed = unpack_pixel_count(ai, frame, pal_translate[value], count, aabitmap, bpp);
				} else {					
					stuffed = unpack_pixel_count(ai, frame, (ubyte)value, count, aabitmap, bpp);
				}

				frame += stuffed;
				size -= count;
			}
		}
	}
	else if ( *ptr == PACKING_METHOD_STD_RLE_KEY) {	// key frame, with high bit as count
		ptr++;
		while (size > 0) {
			value = *ptr++;
			if ( !(value & STD_RLE_CODE) ) {
				if ( xlate_pal ){
					stuffed = unpack_pixel(ai, frame, pal_translate[value], aabitmap, bpp);
				} else {
					stuffed = unpack_pixel(ai, frame, (ubyte)value, aabitmap, bpp);
				}

				frame += stuffed;
				size--;
			} else {
				count = value & (~STD_RLE_CODE);
				value = *ptr++;

				if (count > size)
					count = size;

				size -= count;
				Assert(size >= 0);

				if ( xlate_pal ){
					stuffed = unpack_pixel_count(ai, frame, pal_translate[value], count, aabitmap, bpp);
				} else {
					stuffed = unpack_pixel_count(ai, frame, (ubyte)value, count, aabitmap, bpp);
				}

				frame += stuffed;
			}
		}
	}
	else if (*ptr == PACKING_METHOD_RLE) {  // normal frame, Hoffoss's RLE format
	
		ptr++;
		while (size > 0) {
			value = *ptr++;
			if (value != packer_code) {
				if (value != transparent_code) {
					if ( xlate_pal ){
						stuffed = unpack_pixel(ai, frame, pal_translate[value], aabitmap, bpp);
					} else {
						stuffed = unpack_pixel(ai, frame, (ubyte)value, aabitmap, bpp);
					}
				} else {
					// temporary pixel
					stuffed = pixel_size;
				}

				frame += stuffed;
				size--;
			} else {
				count = *ptr++;
				if (count < 2){
					value = packer_code;
				} else {
					value = *ptr++;
				}

				if (++count > size){
					count = size;
				}

				size -= count;
				Assert(size >= 0);

				if (value != transparent_code ) {
					if ( xlate_pal ) {
						stuffed = unpack_pixel_count(ai, frame, pal_translate[value], count, aabitmap, bpp);
					} else {
						stuffed = unpack_pixel_count(ai, frame, (ubyte)value, count, aabitmap, bpp);
					}
				} else {
					stuffed = count * pixel_size;
				}

				frame += stuffed;
			}
		}

	}
	else if ( *ptr == PACKING_METHOD_STD_RLE) {	// normal frame, with high bit as count
		ptr++;
		while (size > 0) {
			value = *ptr++;
			if ( !(value & STD_RLE_CODE) ) {
				if (value != transparent_code) {
					if ( xlate_pal ){
						stuffed = unpack_pixel(ai, frame, pal_translate[value], aabitmap, bpp);
					} else {
						stuffed = unpack_pixel(ai, frame, (ubyte)value, aabitmap, bpp);
					}
				} else {
					stuffed = pixel_size;
				}

				frame += stuffed;
				size--;
			} else {
				count = value & (~STD_RLE_CODE);
				value = *ptr++;

				if (count > size)
					count = size;

				size -= count;
				Assert(size >= 0);

				if (value != transparent_code) {
					if ( xlate_pal ){
						stuffed = unpack_pixel_count(ai, frame, pal_translate[value], count, aabitmap, bpp);
					} else {
						stuffed = unpack_pixel_count(ai, frame, (ubyte)value, count, aabitmap, bpp);
					}
				} else {					
					stuffed = pixel_size * count;
				}

				frame += stuffed;
			}
		}
	}
	else {
		// unknown packing method
		return NULL;
	}

	return ptr;
}

/**
 * @brief Unpack frame from file
 *
 * @param ai Animation instance
 * @param frame Where to store unpacked data to
 * @param size Total number of unpacked pixels requested
 * @param pal_translate Color translation lookup table (NULL if no palette translation desired)
 * @param aabitmap
 * @param bpp
 */
int unpack_frame_from_file(anim_instance *ai, ubyte *frame, int size, ubyte *pal_translate, int aabitmap, int bpp)
{
	int	xlate_pal, value, count = 0;
	int	offset = 0;
	int stuffed;	
	int pixel_size = (bpp / 8);

	if ( pal_translate == NULL ) {
		xlate_pal = 0;
	}
	else {
		xlate_pal = 1;
	}

	if (anim_instance_get_byte(ai,offset) == PACKING_METHOD_RLE_KEY) {  // key frame, Hoffoss's RLE format
		offset++;
		while (size > 0) {
			value = anim_instance_get_byte(ai,offset);
			offset++;
			if (value != packer_code) {
				if ( xlate_pal ){
					stuffed = unpack_pixel(ai, frame, pal_translate[value], aabitmap, bpp);
				} else {
					stuffed = unpack_pixel(ai, frame, (ubyte)value, aabitmap, bpp);
				}

				frame += stuffed;
				size--;
			} else {
				count = anim_instance_get_byte(ai,offset);
				offset++;
				if (count < 2) {
					value = packer_code;
				} else {
					value = anim_instance_get_byte(ai,offset);
					offset++;
				}

				if (++count > size){
					count = size;
				}

				if ( xlate_pal ){
					stuffed = unpack_pixel_count(ai, frame, pal_translate[value], count, aabitmap, bpp);
				} else {
					stuffed = unpack_pixel_count(ai, frame, (ubyte)value, count, aabitmap, bpp);
				}

				frame += stuffed;
				size -= count;
			}
		}
	}
	else if ( anim_instance_get_byte(ai,offset) == PACKING_METHOD_STD_RLE_KEY) {	// key frame, with high bit as count
		offset++;
		while (size > 0) {
			value = anim_instance_get_byte(ai,offset);
			offset++;
			if ( !(value & STD_RLE_CODE) ) {
				if ( xlate_pal ){
					stuffed = unpack_pixel(ai, frame, pal_translate[value], aabitmap, bpp);
				} else {
					stuffed = unpack_pixel(ai, frame, (ubyte)value, aabitmap, bpp);
				}

				frame += stuffed;
				size--;
			} else {
				count = value & (~STD_RLE_CODE);
				value = anim_instance_get_byte(ai,offset);
				offset++;

				if (count > size)
					count = size;

				size -= count;
				Assert(size >= 0);

				if ( xlate_pal ){
					stuffed = unpack_pixel_count(ai, frame, pal_translate[value], count, aabitmap, bpp);
				} else {
					stuffed = unpack_pixel_count(ai, frame, (ubyte)value, count, aabitmap, bpp);
				}

				frame += stuffed;
			}
		}
	}
	else if (anim_instance_get_byte(ai,offset) == PACKING_METHOD_RLE) {  // normal frame, Hoffoss's RLE format
	
		offset++;
		while (size > 0) {
			value = anim_instance_get_byte(ai,offset);
			offset++;
			if (value != packer_code) {
				if (value != transparent_code) {
					if ( xlate_pal ){
						stuffed = unpack_pixel(ai, frame, pal_translate[value], aabitmap, bpp);
					} else {
						stuffed = unpack_pixel(ai, frame, (ubyte)value, aabitmap, bpp);
					}
				} else {
					stuffed = pixel_size;
				}

				frame += stuffed;
				size--;
			} else {
				count = anim_instance_get_byte(ai,offset);
				offset++;

				if (count < 2) {
					value = packer_code;
				} else {
					value = anim_instance_get_byte(ai,offset);
					offset++;
				}
				if (++count > size){
					count = size;
				}

				size -= count;
				Assert(size >= 0);

				if (value != transparent_code ) {
					if ( xlate_pal ) {
						stuffed = unpack_pixel_count(ai, frame, pal_translate[value], count, aabitmap, bpp);
					} else {
						stuffed = unpack_pixel_count(ai, frame, (ubyte)value, count, aabitmap, bpp);
					}
				} else {
					stuffed = pixel_size * count;
				}

				frame += stuffed;
			}
		}

	}
	else if ( anim_instance_get_byte(ai,offset) ) {	// normal frame, with high bit as count
		offset++;
		while (size > 0) {
			value = anim_instance_get_byte(ai,offset);
			offset++;
			if ( !(value & STD_RLE_CODE) ) {
				if (value != transparent_code) {
					if ( xlate_pal ){
						stuffed = unpack_pixel(ai, frame, pal_translate[value], aabitmap, bpp);
					} else {
						stuffed = unpack_pixel(ai, frame, (ubyte)value, aabitmap, bpp);
					}
				} else {
					stuffed = pixel_size;
				}

				frame += stuffed;
				size--;
			} else {
				count = value & (~STD_RLE_CODE);
				value = anim_instance_get_byte(ai,offset);
				offset++;

				if (count > size)
					count = size;

				size -= count;
				Assert(size >= 0);

				if (value != transparent_code) {
					if ( xlate_pal ){
						stuffed = unpack_pixel_count(ai, frame, pal_translate[value], count, aabitmap, bpp);
					} else {
						stuffed = unpack_pixel_count(ai, frame, (ubyte)value, count, aabitmap, bpp);
					}
				} else {					
					stuffed = pixel_size * count;
				}

				frame += stuffed;
			}
		}
	}
	else {
		// unknown packing method
		return -1;
	}

	return ai->file_offset + offset;
}


/**
 * @brief Set animation palette
 * @todo Actually convert the frame data to correct palette at this point
 */
void anim_set_palette(anim *ptr)
{
	int i, xparent_found = 0;
	
	// create the palette translation look-up table
	for ( i = 0; i < 256; i++ ) {
		ptr->palette_translation[i] = (ubyte)i;
	}	

	if ( xparent_found ) {
		ptr->flags |= ANF_XPARENT;
	}
	else {
		ptr->flags &= ~ANF_XPARENT;
	}
}
