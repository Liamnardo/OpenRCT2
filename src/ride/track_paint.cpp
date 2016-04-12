/*****************************************************************************
* Copyright (c) 2014 Ted John
* OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
*
* This file is part of OpenRCT2.
*
* OpenRCT2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

extern "C"
{
    #include "../addresses.h"
    #include "../config.h"
    #include "../drawing/drawing.h"
    #include "../drawing/supports.h"
    #include "../interface/viewport.h"
    #include "../interface/window.h"
    #include "../localisation/localisation.h"
    #include "../sprites.h"
    #include "../world/map.h"
    #include "../world/sprite.h"
    #include "ride_data.h"
    #include "track_data.h"
    #include "track_paint.h"
}

enum {
    DF_NE = 1 << 0,
    DF_SE = 1 << 1,
    DF_SW = 1 << 2,
    DF_NW = 1 << 3,
};

int TileDrawingContext::draw_98197C(uint32 imageId, sint8 offsetX, sint8 offsetY, sint16 lengthX, sint16 lengthY, sint8 offsetZ, sint32 height)
{
    return sub_98197C(imageId, offsetX, offsetY, lengthX, lengthY, offsetZ, height, ViewRotation);
}

int TileDrawingContext::draw_98199C(uint32 imageId, sint8 offsetX, sint8 offsetY, sint16 lengthX, sint16 lengthY, sint8 offsetZ, sint32 height)
{
    return sub_98199C(imageId, offsetX, offsetY, lengthX, lengthY, offsetZ, height, ViewRotation);
}

int TileDrawingContext::draw_98199C(uint32 imageId, sint8 offsetX, sint8 offsetY, sint16 lengthX, sint16 lengthY, sint8 offsetZ, sint32 height, uint8 rotation)
{
    return sub_98199C(imageId, offsetX, offsetY, lengthX, lengthY, offsetZ, height, rotation);
}

void TileDrawingContext::UpdateTileMaxHeight(sint16 height, uint8 byte_0141E9DA)
{
    if (RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_PAINT_TILE_MAX_HEIGHT, sint16) < height)
    {
        RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_PAINT_TILE_MAX_HEIGHT, sint16) = height;
        RCT2_GLOBAL(0x0141E9DA, uint8) = 32;
    }
}

static rct_sxy8 GetEntranceCheckOffset(uint8 direction, uint8 rotation)
{
    const static rct_sxy8 entranceCheckOffsets[] = {
        { -1,  0 },
        {  0, -1 },
        {  1,  0 },
        {  0,  1 },

        {  0,  1 },
        { -1,  0 },
        {  0, -1 },
        {  1,  0 },

        {  1,  0 },
        {  0,  1 },
        { -1,  0 },
        {  0, -1 },

        {  0, -1 },
        {  1,  0 },
        {  0,  1 },
        { -1,  0 },
    };

#if DEBUG
    assert(direction <= 3);
    assert(rotation <= 3);
#endif
    return entranceCheckOffsets[(direction * 4) + rotation];
}

void RideDrawingContext::DrawFloor(uint8 floorType, uint32 imageFlags, sint32 z)
{
    uint32 imageId = (22134 + Direction) | imageFlags;
    RCT2_GLOBAL(0x009DEA52, uint16) = 0;
    RCT2_GLOBAL(0x009DEA54, uint16) = 0;
    RCT2_GLOBAL(0x009DEA56, uint16) = z;
    draw_98197C(imageId, 0, 0, 32, 32, 1, z);
}

void RideDrawingContext::DrawFence(uint8 fenceType, sint32 z)
{
    uint32 imageId;
    switch (Direction) {
    case 0:
        imageId = 20564 | RCT2_GLOBAL(0x00F44198, uint32);
        RCT2_GLOBAL(0x009DEA52, uint16) = 2;
        RCT2_GLOBAL(0x009DEA54, uint16) = 0;
        RCT2_GLOBAL(0x009DEA56, uint16) = z + 2;
        draw_98199C(imageId, 0, 0, 1, 32, 7, z + 2);
        break;
    case 1:
        imageId = 20565 | RCT2_GLOBAL(0x00F44198, uint32);
        RCT2_GLOBAL(0x009DEA52, uint16) = 0;
        RCT2_GLOBAL(0x009DEA54, uint16) = 30;
        RCT2_GLOBAL(0x009DEA56, uint16) = z + 2;
        draw_98199C(imageId, 0, 0, 32, 1, 7, z + 2);
        break;
    case 2:
        imageId = 20566 | RCT2_GLOBAL(0x00F44198, uint32);
        RCT2_GLOBAL(0x009DEA52, uint16) = 30;
        RCT2_GLOBAL(0x009DEA54, uint16) = 0;
        RCT2_GLOBAL(0x009DEA56, uint16) = z + 2;
        draw_98199C(imageId, 0, 0, 1, 32, 7, z + 2);
        break;
    case 3:
        imageId = 20567 | RCT2_GLOBAL(0x00F44198, uint32);
        RCT2_GLOBAL(0x009DEA52, uint16) = 0;
        RCT2_GLOBAL(0x009DEA54, uint16) = 2;
        RCT2_GLOBAL(0x009DEA56, uint16) = z + 2;
        draw_98199C(imageId, 0, 0, 32, 1, 7, z + 2);
        break;
    }
}

void RideDrawingContext::DrawFenceChecked(uint8 fenceType, sint32 z)
{
    uint8 currentRotation = get_current_rotation();
    rct_sxy8 checkOffset = GetEntranceCheckOffset(Direction, ViewRotation);

    rct_xy8 fenceCheckPosition;
    fenceCheckPosition.x = RCT2_GLOBAL(0x009DE56A, uint16) >> 5;
    fenceCheckPosition.y = RCT2_GLOBAL(0x009DE56E, uint16) >> 5;
    fenceCheckPosition.x += checkOffset.x;
    fenceCheckPosition.y += checkOffset.y;

    uint8 stationId = map_get_station(MapElement);
    rct_ride *ride = get_ride(MapElement->properties.track.ride_index);
    if (fenceCheckPosition.xy != ride->entrances[stationId] &&
        fenceCheckPosition.xy != ride->exits[stationId])
    {
        DrawFence(Direction, z);
    }
}

void RideDrawingContext::DrawFencesChecked(uint8 fenceDirections, uint8 fenceType, sint32 z)
{
    for (int i = 0; i < 4; i++)
    {
        if (fenceDirections & (1 << i))
        {
            DrawFenceChecked((Direction + i) & 3, z);
        }
    }
}

namespace TopSpin
{
    /**
     *
     *  rct2: 0x0076750D
     */
    static void DrawVehicle(RideDrawingContext * dc, sint8 al, sint8 cl, sint32 height)
    {
        /**
         *
         *  rct2: 0x0142811C
         * Can be calculated as Rounddown(34*sin(x)+0.5)
         * where x is in 7.5 deg segments.
         */
        const static sint8 TopSpinSeatPositionOffset[] = {
              0,   4,   9,  13,  17,  21,  24,  27,  29,  31,  33,  34,  34,  34,  33,  31,
             29,  27,  24,  21,  17,  13,   9,   4,   0,  -3,  -8, -12, -16, -20, -23, -26,
            -28, -30, -32, -33, -33, -33, -32, -30, -28, -26, -23, -20, -16, -12,  -8,  -3,
              0
        };

        // As we will be drawing a vehicle we need to backup the mapElement that
        // is assigned to the drawings.
        rct_map_element * curMapElement = RCT2_GLOBAL(0x009DE578, rct_map_element*);

        height += 3;

        rct_vehicle * vehicle = NULL;

        uint8 seatRotation = 0;
        sint8 armRotation = 0;

        if (dc->Ride->lifecycle_flags & RIDE_LIFECYCLE_ON_TRACK &&
            dc->Ride->vehicles[0] != SPRITE_INDEX_NULL)
        {
            vehicle = GET_VEHICLE(dc->Ride->vehicles[0]);

            RCT2_GLOBAL(RCT2_ADDRESS_PAINT_SETUP_CURRENT_TYPE, uint8) = VIEWPORT_INTERACTION_ITEM_SPRITE;
            RCT2_GLOBAL(0x009DE578, rct_vehicle*) = vehicle;

            armRotation = vehicle->vehicle_sprite_type;
            seatRotation = vehicle->bank_rotation;
        }

        RCT2_GLOBAL(0x009DEA52, sint16) = al + 16;
        RCT2_GLOBAL(0x009DEA54, sint16) = cl + 16;
        RCT2_GLOBAL(0x009DEA56, sint16) = height;

        uint8 lengthX = 24; // di
        uint8 lengthY = 24; // si

        uint32 image_id = RCT2_GLOBAL(0x00F441A0, uint32);
        if (image_id == 0x20000000)
        {
            image_id = 0xA0000000 |
                       (dc->Ride->track_colour_main[0] << 19) |
                       (dc->Ride->track_colour_supports[0] << 24);
        }

        image_id += (dc->Direction & 1) << 1;
        image_id += dc->RideEntry->vehicles[0].base_image_id;
        // Left back bottom support
        image_id += 572;

        dc->draw_98197C(image_id, al, cl, lengthX, lengthY, 90, height);

        image_id = RCT2_GLOBAL(0x00F441A0, uint32);
        if (image_id == 0x20000000) {
            image_id = 0xA0000000 |
                       (dc->Ride->track_colour_main[0] << 19) |
                       (dc->Ride->track_colour_additional[0] << 24);
        }

        sint32 var_1F = armRotation;
        if (dc->Direction & 2)
        {
            var_1F = -var_1F;
            if (var_1F != 0)
            {
                var_1F += 48;
            }
        }
        image_id += var_1F;
        image_id += (dc->Direction & 1) * 48;
        image_id += dc->RideEntry->vehicles[0].base_image_id;
        // Left hand arm
        image_id += 380;

        dc->draw_98199C(image_id,
                       al,
                       cl,
                       lengthX,
                       lengthY,
                       90,
                       height,
                       0);

        uint32 seatImageId;

        if (vehicle != NULL && vehicle->restraints_position >= 64)
        {
            // Open Restraints
            image_id = (vehicle->restraints_position - 64) >> 6;
            image_id += dc->Direction * 3;
            image_id += dc->RideEntry->vehicles[0].base_image_id;
            image_id += 64;
            seatImageId = image_id;
        }
        else
        {
            image_id = dc->Direction * 16;
            // Var_20 Rotation of seats
            image_id += seatRotation;
            image_id += dc->RideEntry->vehicles[0].base_image_id;
            seatImageId = image_id;
        }

        image_id = RCT2_GLOBAL(0x00F441A0, uint32);
        if (image_id == 0x20000000)
        {
            image_id = 0xA0000000 |
                       (dc->Ride->vehicle_colours[0].body_colour << 19) |
                       (dc->Ride->vehicle_colours[0].trim_colour << 24);
        }
        image_id += seatImageId;

        rct_xyz16 seatCoords;
        seatCoords.x = al;
        seatCoords.y = cl;
        seatCoords.z = height;
        seatCoords.z += RCT2_ADDRESS(0x14280BC, sint16)[armRotation];

        assert(armRotation < sizeof(TopSpinSeatPositionOffset));
        switch (dc->Direction) {
        case 0:
            seatCoords.x -= TopSpinSeatPositionOffset[armRotation];
            break;
        case 1:
            seatCoords.y += TopSpinSeatPositionOffset[armRotation];
            break;
        case 2:
            seatCoords.x += TopSpinSeatPositionOffset[armRotation];
            break;
        case 3:
            seatCoords.y -= TopSpinSeatPositionOffset[armRotation];
            break;
        }

        RCT2_GLOBAL(0x014280B8, sint8) = (sint8)seatCoords.x;
        RCT2_GLOBAL(0x014280B9, sint8) = (sint8)seatCoords.y;
        RCT2_GLOBAL(0x014280BA, sint16) = seatCoords.z;

        dc->draw_98199C(image_id,
                       (sint8)seatCoords.x,
                       (sint8)seatCoords.y,
                       lengthX,
                       lengthY,
                       90,
                       seatCoords.z,
                       0);

        rct_drawpixelinfo * dpi = RCT2_GLOBAL(0x140E9A8, rct_drawpixelinfo*);
        if (dpi->zoom_level < 2 && vehicle != NULL && vehicle->num_peeps != 0)
        {
            image_id = (vehicle->peep_tshirt_colours[0] << 19) |
                       (vehicle->peep_tshirt_colours[1] << 24);
            image_id += seatImageId;
            image_id += 0xA0000000;
            image_id += 76;

            dc->draw_98199C(image_id, (sint8)seatCoords.x, (sint8)seatCoords.y, lengthX, lengthY, 90, seatCoords.z, 0);

            if (vehicle->num_peeps > 2)
            {
                image_id = (vehicle->peep_tshirt_colours[2] << 19) |
                           (vehicle->peep_tshirt_colours[3] << 24);
                image_id += seatImageId;
                image_id += 0xA0000000;
                image_id += 152;

                dc->draw_98199C(image_id, (sint8)seatCoords.x, (sint8)seatCoords.y, lengthX, lengthY, 90, seatCoords.z, 0);
            }

            if (vehicle->num_peeps > 4)
            {
                image_id = (vehicle->peep_tshirt_colours[4] << 19) |
                           (vehicle->peep_tshirt_colours[5] << 24);
                image_id += seatImageId;
                image_id += 0xA0000000;
                image_id += 228;

                dc->draw_98199C(image_id, (sint8)seatCoords.x, (sint8)seatCoords.y, lengthX, lengthY, 90, seatCoords.z, 0);
            }

            if (vehicle->num_peeps > 6)
            {
                image_id = (vehicle->peep_tshirt_colours[6] << 19) |
                           (vehicle->peep_tshirt_colours[7] << 24);
                image_id += seatImageId;
                image_id += 0xA0000000;
                image_id += 304;

                dc->draw_98199C(image_id, (sint8)seatCoords.x, (sint8)seatCoords.y, lengthX, lengthY, 90, seatCoords.z, 0);
            }
        }

        image_id = RCT2_GLOBAL(0x00F441A0, uint32);
        if (image_id == 0x20000000)
        {
            image_id = 0xA0000000 |
                       (dc->Ride->track_colour_main[0] << 19) |
                       (dc->Ride->track_colour_additional[0] << 24);
        }

        image_id += var_1F;
        image_id += (dc->Direction & 1) * 48;
        image_id += dc->RideEntry->vehicles[0].base_image_id;
        // Right hand arm
        image_id += 476;

        dc->draw_98199C(image_id,
                       al,
                       cl,
                       lengthX,
                       lengthY,
                       90,
                       height,
                       0);

        image_id = RCT2_GLOBAL(0x00F441A0, uint32);
        if (image_id == 0x20000000)
        {
            image_id = 0xA0000000 |
                       (dc->Ride->track_colour_main[0] << 19) |
                       (dc->Ride->track_colour_supports[0] << 24);
        }

        image_id += (dc->Direction & 1) << 1;
        image_id += dc->RideEntry->vehicles[0].base_image_id;
        // Right back bottom support
        image_id += 573;

        dc->draw_98199C(image_id,
                       al,
                       cl,
                       lengthX,
                       lengthY,
                       90,
                       height,
                       0);

        RCT2_GLOBAL(0x009DE578, rct_map_element*) = curMapElement;
        RCT2_GLOBAL(RCT2_ADDRESS_PAINT_SETUP_CURRENT_TYPE, uint8) = VIEWPORT_INTERACTION_ITEM_RIDE;
    }

    static uint16 TransformArg(uint16 arg, sint32 height)
    {
        if (arg == 2) return height + 2;
        return arg;
    }

    /**
     *
     *  rct2: 0x0076659C
     */
    static void Draw(RideDrawingContext * dc)
    {
        if (dc->TrackType != 123) return;
        if (dc->TrackSequence > 8) return;

        struct TopSpinTileInfo
        {
            uint8 fences;
            sint8 vehicle_offset_x;
            sint8 vehicle_offset_y;
            uint8 max_height;
            uint8 mapDirection[4];
            uint16 unk_args[12];
        };

        const static TopSpinTileInfo TopSpinTrackSeqFenceMap[] = {
            { 0,                  0,   0, 112, 0, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
            { DF_NE | DF_NW,     32,  32, 110, 1, 3, 7, 6, 2,          32, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,      2,     32,      2,     32, 0xFFFF, 0xFFFF },
            { DF_NE,              0,   0, 110, 2, 5, 8, 4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
            { DF_NE | DF_SE,     32, -32, 110, 3, 7, 6, 1, 0xFFFF, 0xFFFF,      2,     32, 0xFFFF, 0xFFFF, 0xFFFF,      2,     32, 0xFFFF,      2,     32 },
            { DF_NW,              0,   0, 110, 4, 2, 5, 8, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
            { DF_SE,              0, -32, 112, 5, 8, 4, 2, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
            { DF_SW | DF_NW,    -32,  32, 110, 6, 1, 3, 7, 0xFFFF,      2,     32, 0xFFFF, 0xFFFF, 0xFFFF,      2,     32, 0xFFFF,      2,     32, 0xFFFF },
            { DF_SW | DF_SE,    -32, -32, 110, 7, 6, 1, 3, 0xFFFF, 0xFFFF, 0xFFFF,      2,     32, 0xFFFF, 0xFFFF, 0xFFFF,      2,     32,      2,     32 },
            { DF_SW,            -32,   0, 112, 8, 4, 2, 5, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
        };

        wooden_a_supports_paint_setup(dc->Direction & 1, 0, dc->Z, RCT2_GLOBAL(0x00F441A0, uint32), NULL);
        dc->DrawFloor(0, RCT2_GLOBAL(0x00F44198, uint32), dc->Z);
        uint8 fences = TopSpinTrackSeqFenceMap[dc->TrackSequence].fences;
        dc->DrawFencesChecked(fences, 0, dc->Z);

        int correctedSequence = TopSpinTrackSeqFenceMap[dc->TrackSequence].mapDirection[dc->Direction];
        const TopSpinTileInfo * ti = &TopSpinTrackSeqFenceMap[correctedSequence];

        if (ti->vehicle_offset_x != 0 || ti->vehicle_offset_y != 0)
        {
            DrawVehicle(dc, ti->vehicle_offset_x, ti->vehicle_offset_y, dc->Z);
        }

        RCT2_GLOBAL(0x141E9B4, uint16) = TransformArg(ti->unk_args[ 0], dc->Z);
        RCT2_GLOBAL(0x141E9B6, uint16) = TransformArg(ti->unk_args[ 1], dc->Z);
        RCT2_GLOBAL(0x141E9B8, uint16) = TransformArg(ti->unk_args[ 2], dc->Z);
        RCT2_GLOBAL(0x141E9BC, uint16) = TransformArg(ti->unk_args[ 3], dc->Z);
        RCT2_GLOBAL(0x141E9C0, uint16) = TransformArg(ti->unk_args[ 4], dc->Z);
        RCT2_GLOBAL(0x141E9C4, uint16) = TransformArg(ti->unk_args[ 5], dc->Z);
        RCT2_GLOBAL(0x141E9C8, uint16) = TransformArg(ti->unk_args[ 6], dc->Z);
        RCT2_GLOBAL(0x141E9CA, uint16) = TransformArg(ti->unk_args[ 7], dc->Z);
        RCT2_GLOBAL(0x141E9CC, uint16) = TransformArg(ti->unk_args[ 8], dc->Z);
        RCT2_GLOBAL(0x141E9CE, uint16) = TransformArg(ti->unk_args[ 9], dc->Z);
        RCT2_GLOBAL(0x141E9D0, uint16) = TransformArg(ti->unk_args[10], dc->Z);
        RCT2_GLOBAL(0x141E9D4, uint16) = TransformArg(ti->unk_args[11], dc->Z);

        dc->UpdateTileMaxHeight(dc->Z + ti->max_height, 32);
    }
}

namespace Shop
{
    /**
     *
     *  rct2: 0x00761160
     */
    static void Draw(RideDrawingContext * dc)
    {
        if (dc->TrackType != 118 &&
            dc->TrackType != 121) return;

        bool hasSupports = wooden_a_supports_paint_setup(dc->Direction & 1, 0, dc->Z, RCT2_GLOBAL(0x00F441A4, uint32), NULL);

        RCT2_GLOBAL(0x0141E9D0, sint16) = -1;
        RCT2_GLOBAL(0x0141E9C4, sint16) = -1;
        RCT2_GLOBAL(0x0141E9CC, sint16) = -1;
        RCT2_GLOBAL(0x0141E9B8, sint16) = -1;
        RCT2_GLOBAL(0x0141E9BC, sint16) = -1;
        RCT2_GLOBAL(0x0141E9B4, sint16) = -1;
        RCT2_GLOBAL(0x0141E9C0, sint16) = -1;
        RCT2_GLOBAL(0x0141E9C8, sint16) = -1;
        RCT2_GLOBAL(0x0141E9D4, sint16) = -1;

        rct_ride_entry_vehicle *firstVehicleEntry = &dc->RideEntry->vehicles[0];

        uint32 imageId = RCT2_GLOBAL(0x00F44198, uint32);
        if (imageId & 0x80000000) {
            imageId &= 0x60FFFFFF;
        }
        imageId += firstVehicleEntry->base_image_id;
        imageId += dc->Direction;

        sint16 height16 = (sint16)dc->Z;
        if (hasSupports) {
            uint32 foundationImageId = RCT2_GLOBAL(0x00F441A4, uint32);
            foundationImageId |= 3395;

            RCT2_GLOBAL(0x009DEA52, uint16) = 2;
            RCT2_GLOBAL(0x009DEA54, uint16) = 2;
            RCT2_GLOBAL(0x009DEA56, sint16) = height16;
            dc->draw_98197C(foundationImageId, 0, 0, 28, 28, 45, dc->Z);

            RCT2_GLOBAL(0x009DEA52, uint16) = 2;
            RCT2_GLOBAL(0x009DEA54, uint16) = 2;
            RCT2_GLOBAL(0x009DEA56, sint16) = height16;
            dc->draw_98199C(imageId, 0, 0, 28, 28, 45, dc->Z);
        } else {
            RCT2_GLOBAL(0x009DEA52, uint16) = 2;
            RCT2_GLOBAL(0x009DEA54, uint16) = 2;
            RCT2_GLOBAL(0x009DEA56, sint16) = height16;
            dc->draw_98197C(imageId, 0, 0, 28, 28, 45, dc->Z);
        }

        dc->UpdateTileMaxHeight(height16 + 48, 32);
    }
}

namespace Facility
{
    /**
     *
     *  rct2: 0x00762D44
     */
    static void Draw(RideDrawingContext * dc)
    {
        if (dc->TrackType != 118) return;

        bool hasSupports = wooden_a_supports_paint_setup(dc->Direction & 1, 0, dc->Z, RCT2_GLOBAL(0x00F441A4, uint32), NULL);

        RCT2_GLOBAL(0x0141E9D0, sint16) = -1;
        RCT2_GLOBAL(0x0141E9C4, sint16) = -1;
        RCT2_GLOBAL(0x0141E9CC, sint16) = -1;
        RCT2_GLOBAL(0x0141E9B8, sint16) = -1;
        RCT2_GLOBAL(0x0141E9BC, sint16) = -1;
        RCT2_GLOBAL(0x0141E9B4, sint16) = -1;
        RCT2_GLOBAL(0x0141E9C0, sint16) = -1;
        RCT2_GLOBAL(0x0141E9C8, sint16) = -1;
        RCT2_GLOBAL(0x0141E9D4, sint16) = -1;

        rct_ride_entry_vehicle * firstVehicleEntry = &dc->RideEntry->vehicles[0];

        uint32 imageId = RCT2_GLOBAL(0x00F44198, uint32);
        imageId |= firstVehicleEntry->base_image_id;
        imageId += (dc->Direction + 2) & 3;

        sint16 height16 = (sint16)dc->Z;
        int lengthX = (dc->Direction & 1) == 0 ? 28 : 2;
        int lengthY = (dc->Direction & 1) == 0 ? 2 : 28;
        if (hasSupports) {
            uint32 foundationImageId = RCT2_GLOBAL(0x00F441A4, uint32);
            foundationImageId |= 3395;

            RCT2_GLOBAL(0x009DEA52, uint16) = dc->Direction == 3 ? 28 : 2;
            RCT2_GLOBAL(0x009DEA54, uint16) = dc->Direction == 0 ? 28 : 2;
            RCT2_GLOBAL(0x009DEA56, sint16) = height16;
            dc->draw_98197C(foundationImageId, 0, 0, lengthX, lengthY, 29, dc->Z);

            // Door image or base
            RCT2_GLOBAL(0x009DEA52, uint16) = dc->Direction == 3 ? 28 : 2;
            RCT2_GLOBAL(0x009DEA54, uint16) = dc->Direction == 0 ? 28 : 2;
            RCT2_GLOBAL(0x009DEA56, sint16) = height16;
            dc->draw_98199C(imageId, 0, 0, lengthX, lengthY, 29, dc->Z);
        } else {
            // Door image or base
            RCT2_GLOBAL(0x009DEA52, uint16) = dc->Direction == 3 ? 28 : 2;
            RCT2_GLOBAL(0x009DEA54, uint16) = dc->Direction == 0 ? 28 : 2;
            RCT2_GLOBAL(0x009DEA56, sint16) = height16;

            dc->draw_98197C(imageId, 0, 0, lengthX, lengthY, 29, dc->Z);
        }

        // Base image if door was drawn
        if (dc->Direction == 1) {
            imageId += 2;
            RCT2_GLOBAL(0x009DEA52, uint16) = 28;
            RCT2_GLOBAL(0x009DEA54, uint16) = 2;
            RCT2_GLOBAL(0x009DEA56, sint16) = height16;
            dc->draw_98197C(imageId, 0, 0, 2, 28, 29, dc->Z);
        } else if (dc->Direction == 2) {
            imageId += 4;
            RCT2_GLOBAL(0x009DEA52, uint16) = 2;
            RCT2_GLOBAL(0x009DEA54, uint16) = 28;
            RCT2_GLOBAL(0x009DEA56, sint16) = height16;
            dc->draw_98197C(imageId, 0, 0, 28, 2, 29, dc->Z);
        }

        dc->UpdateTileMaxHeight(height16 + 32, 32);
    }
}

namespace MotionSimulator
{
    static void DrawSimulator(RideDrawingContext * dc, uint32 imageId, sint32 offsetX, sint32 offsetY, sint32 z, bool is9C)
    {
        if (is9C)
        {
            dc->draw_98199C(imageId, offsetX, offsetY, 20, 20, 44, z);
        } else
        {
            dc->draw_98197C(imageId, offsetX, offsetY, 20, 20, 44, z);
        }
    }

    static void DrawStairs(RideDrawingContext * dc, sint32 offsetX, sint32 offsetY, sint32 z, bool is9C)
    {
        uint32 imageId = (22154 + dc->Direction) | RCT2_GLOBAL(0x00F441A0, uint32);
        if (is9C)
        {
            dc->draw_98199C(imageId, offsetX, offsetY, 20, 20, 44, z);
        } else
        {
            dc->draw_98197C(imageId, offsetX, offsetY, 20, 20, 44, z);
        }
    }

    static void DrawStairsRails(RideDrawingContext * dc, sint32 offsetX, sint32 offsetY, sint32 z, bool is9C)
    {
        uint32 imageId = (22158 + dc->Direction) | RCT2_GLOBAL(0x00F441A0, uint32);
        if (is9C)
        {
            dc->draw_98199C(imageId, offsetX, offsetY, 20, 2, 44, z);
        } else
        {
            dc->draw_98197C(imageId, offsetX, offsetY, 20, 2, 44, z);
        }
    }

    /**
     *
     *  rct2: 0x0076522A
     */
    static void DrawVehicle(RideDrawingContext * dc, sint8 offsetX, sint8 offsetY)
    {
        uint32 backup_dword_9DE578 = RCT2_GLOBAL(0x009DE578, uint32);
        // push dword_9DE578
        sint32 z = dc->Z + 2;
        rct_vehicle *vehicle = NULL;
        if (dc->Ride->lifecycle_flags & RIDE_LIFECYCLE_ON_TRACK) {
            uint16 spriteIndex = dc->Ride->vehicles[0];
            if (spriteIndex != SPRITE_INDEX_NULL) {
                vehicle = GET_VEHICLE(spriteIndex);
                RCT2_GLOBAL(0x009DE570, uint8) = 2;
                RCT2_GLOBAL(0x009DE578, rct_vehicle*) = vehicle;
            }
        }

        uint32 imageId = dc->RideEntry->vehicles[0].base_image_id + dc->Direction;
        if (vehicle != NULL) {
            if (vehicle->restraints_position >= 64) {
                imageId += (vehicle->restraints_position >> 6) << 2;
            } else {
                imageId += vehicle->vehicle_sprite_type * 4;
            }
        }

        RCT2_GLOBAL(0x01428090, uint32) = imageId;
        RCT2_GLOBAL(0x01428094, uint32) = dc->Direction;
        imageId = RCT2_GLOBAL(0x00F441A0, uint32);
        if (imageId == 0x20000000) {
            imageId = (IMAGE_TYPE_UNKNOWN | IMAGE_TYPE_USE_PALETTE) << 28;
            imageId |= dc->Ride->vehicle_colours[0].trim_colour << 19;
            imageId |= dc->Ride->vehicle_colours[0].body_colour << 24;
        }
        imageId |= RCT2_GLOBAL(0x01428090, uint32);
        uint32 simulatorImageId = imageId;

        RCT2_GLOBAL(0x009DEA52, sint16) = offsetX;
        RCT2_GLOBAL(0x009DEA54, sint16) = offsetY;
        RCT2_GLOBAL(0x009DEA56, sint16) = z;

        switch (dc->Direction) {
        case 0:
            DrawSimulator(dc, simulatorImageId, offsetX, offsetY, z, false);
            DrawStairs(dc, offsetX, offsetY, z, true);
            RCT2_GLOBAL(0x009DEA54, uint16) += 32;
            DrawStairsRails(dc, offsetX, offsetY, z, false);
            break;
        case 1:
            DrawSimulator(dc, simulatorImageId, offsetX, offsetY, z, false);
            DrawStairs(dc, offsetX, offsetY, z, true);
            RCT2_GLOBAL(0x009DEA52, uint16) += 34;
            DrawStairsRails(dc, offsetX, offsetY, z, false);
            break;
        case 2:
            RCT2_GLOBAL(0x009DEA54, uint16) -= 10;
            DrawStairsRails(dc, offsetX, offsetY, z, false);
            RCT2_GLOBAL(0x009DEA54, uint16) += 15;
            DrawStairs(dc, offsetX, offsetY, z, true);
            DrawSimulator(dc, simulatorImageId, offsetX, offsetY, z, true);
            break;
        case 3:
            RCT2_GLOBAL(0x009DEA52, uint16) -= 10;
            DrawStairsRails(dc, offsetX, offsetY, z, false);
            RCT2_GLOBAL(0x009DEA52, uint16) += 15;
            DrawStairs(dc, offsetX, offsetY, z, false);
            DrawSimulator(dc, simulatorImageId, offsetX, offsetY, z, true);
            break;
        }

        // pop dword_9DE578
        RCT2_GLOBAL(0x009DE578, uint32) = backup_dword_9DE578;
        RCT2_GLOBAL(0x009DE570, uint8) = 3;
    }

    /**
     *
     *  rct2: 0x00763520
     */
    static void Draw(RideDrawingContext * dc)
    {
        const static struct
        {
            uint8 fences;
            rct_sxy8 offsets[4];
        } DirectionInfo[] =
        {
            { DF_NW | DF_NE,   0,   0,   0,   0,   0,   0,   0,   0 },
            { DF_NE | DF_SE,  16, -16, -16, -16, -16,  16,  16,  16 },
            { DF_NW | DF_SW, -16,  16,  16,  16,  16, -16, -16, -16 },
            { DF_SW | DF_SE, -16, -16, -16,  16,  16,  16,  16, -16 },
        };

        if (dc->TrackType != 110) return;
        if (dc->TrackSequence > 3) return;

        wooden_a_supports_paint_setup(dc->Direction & 1, 0, dc->Z, RCT2_GLOBAL(0x00F441A0, uint32), NULL);
        dc->DrawFloor(0, RCT2_GLOBAL(0x00F4419C, uint32), dc->Z);

        uint8 fences = DirectionInfo[dc->TrackSequence].fences;
        dc->DrawFencesChecked(fences, 0, dc->Z);

        rct_sxy8 offset;
        switch (dc->TrackSequence) {
        case 1:
        case 2:
        case 3:
            offset = DirectionInfo[dc->TrackSequence].offsets[dc->Direction];
            DrawVehicle(dc, offset.x, offset.y);
            break;
        }

        RCT2_GLOBAL(0x0141E9D4, uint16) = 0xFFFF;
        RCT2_GLOBAL(0x0141E9C4, uint16) = 0xFFFF;
        RCT2_GLOBAL(0x0141E9C8, uint16) = 0xFFFF;
        RCT2_GLOBAL(0x0141E9B8, uint16) = 0xFFFF;
        RCT2_GLOBAL(0x0141E9BC, uint16) = 0xFFFF;
        RCT2_GLOBAL(0x0141E9B4, uint16) = 0xFFFF;
        RCT2_GLOBAL(0x0141E9C0, uint16) = 0xFFFF;
        RCT2_GLOBAL(0x0141E9CC, uint16) = 0xFFFF;
        RCT2_GLOBAL(0x0141E9D0, uint16) = 0xFFFF;

        dc->UpdateTileMaxHeight(dc->Z + 128, 32);
    }
}

static RideDrawFunction GetRideDrawFunction(uint8 rideType)
{
    switch (rideType) {
    case RIDE_TYPE_FOOD_STALL:        return Shop::Draw;
    case RIDE_TYPE_1D:                return Shop::Draw;
    case RIDE_TYPE_DRINK_STALL:       return Shop::Draw;
    case RIDE_TYPE_1F:                return Shop::Draw;
    case RIDE_TYPE_SHOP:              return Shop::Draw;
    case RIDE_TYPE_22:                return Shop::Draw;
    case RIDE_TYPE_INFORMATION_KIOSK: return Shop::Draw;
    case RIDE_TYPE_TOILETS:           return Facility::Draw;
    case RIDE_TYPE_MOTION_SIMULATOR:  return MotionSimulator::Draw;
    case RIDE_TYPE_TOP_SPIN:          return TopSpin::Draw;
    case RIDE_TYPE_CASH_MACHINE:      return Shop::Draw;
    case RIDE_TYPE_FIRST_AID:         return Facility::Draw;
    }
    return nullptr;
}

extern "C" void viewport_track_paint_setup_2(uint8 rideIndex, uint8 direction, sint32 height, rct_map_element * mapElement)
{
    rct_ride * ride = get_ride(rideIndex);
    uint8 trackType = mapElement->properties.track.type;
    uint8 trackSequence = mapElement->properties.track.sequence & 0x0F;

    RideDrawFunction drawFunction = GetRideDrawFunction(ride->type);
    if (drawFunction == nullptr)
    {
        TRACK_PAINT_FUNCTION * * trackTypeList = (TRACK_PAINT_FUNCTION**)RideTypeTrackPaintFunctionsOld[ride->type];
        uint32 * trackDirectionList = (uint32*)trackTypeList[trackType];

        // Have to call from this point as it pushes esi and expects callee to pop it
        RCT2_CALLPROC_X(0x006C4934,
                        ride->type,
                        (int)trackDirectionList,
                        direction,
                        height,
                        (int)mapElement,
                        rideIndex * sizeof(rct_ride),
                        trackSequence);
    }
    else
    {
        RideDrawingContext dc;
        dc.ViewRotation = get_current_rotation();
        dc.Direction = direction;
        dc.X = RCT2_GLOBAL(0x009DE56A, uint16);
        dc.X = RCT2_GLOBAL(0x009DE56E, uint16);
        dc.Z = height;
        dc.MapElement = mapElement;
        dc.RideIndex = rideIndex;
        dc.Ride = ride;
        dc.RideEntry = get_ride_entry_by_ride(ride);
        dc.TrackType = trackType;
        dc.TrackSequence = trackSequence;

        drawFunction(&dc);
    }
}
