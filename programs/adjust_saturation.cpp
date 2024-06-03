#include <lcms2.h>
#include <lcms2_plugin.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <stdio.h>

#define DEBUG
//#define SKIP_IF_CLIP

int colorNum = 0;
float minGray = 1.0;
float maxGray = 0.0;
// Function to adjust saturation for normalized float values
void AdjustSaturation(cmsUInt16Number* colors, int numOutputs, double saturation_factor) {
    #ifdef DEBUG
    printf("Adjusting color %d (", ++colorNum);
    #endif

    float gray = 0.0;
    for (int grayAvg = 0; grayAvg < numOutputs; grayAvg++)
    {
       #ifdef DEBUG
       if (grayAvg > 0) printf(",");
       printf("%0.3f", colors[grayAvg] / 65535.0f);
       #endif
       gray += colors[grayAvg] / 65535.0f;
    }
    gray /= (float)numOutputs;
    #ifdef DEBUG
    printf(") gray=%0.4f by scale of %0.2f to be (", gray, saturation_factor);
    #endif
    if (gray < minGray) minGray = gray;
    if (gray > maxGray) maxGray = gray;

    // Check for clipping
    #ifdef SKIP_IF_CLIP
    for (int i = 0; i < numOutputs; i++)
    {
       //unsigned long newVal = gray + saturation_factor * (colors[i] - gray)
       float cVal = colors[i] / 65535.0;
       float newVal = gray + saturation_factor * (cVal - gray);
       if (newVal >= 1.0f) {
          #ifdef DEBUG
          printf(" ... skipping, would clip and change color vs make it more saturated\n");
          #endif
          return;
       }
    }
    #endif
    
    for (int i = 0; i < numOutputs; i++)
    {
       //unsigned long newVal = gray + saturation_factor * (colors[i] - gray)
       float cVal = colors[i] / 65535.0;
       float newVal = gray + saturation_factor * (cVal - gray);
       colors[i] = static_cast<cmsUInt16Number>(std::max(0.0f, std::min(1.0f, newVal)) * 65535.0f);
       #ifdef DEBUG
       if (i > 0) printf(",");
       printf("%0.3f", newVal);
       #endif
    }
    #ifdef DEBUG
    printf(")\n");
    #endif
}

// Function to process CLUT in BToA tags
void ProcessCLUT(cmsHPROFILE hProfile, cmsTagSignature tag, double saturation_factor) {
    cmsPipeline* lut = (cmsPipeline*)cmsReadTag(hProfile, tag);
    if (!lut) {
        std::cerr << "Tag not found: " << tag << "\n";
        return;
    }

    cmsStage* clut_stage = cmsPipelineGetPtrToFirstStage(lut);
    while (clut_stage) {
        if (cmsStageType(clut_stage) == cmsSigCLutElemType) {
            _cmsStageCLutData* clut_data = (_cmsStageCLutData*)cmsStageData(clut_stage);
            size_t clut_size = clut_data->nEntries * clut_data->Params->nOutputs; // total number of color components
            #ifdef DEBUG
            printf("LUT has %lu entries and has TFloat values = %d\n", clut_size, clut_data->HasFloatValues);
            #endif
            for (size_t i = 0; i < clut_size; i += clut_data->Params->nOutputs) {
                if (!clut_data->HasFloatValues)
                {  
                    AdjustSaturation(&clut_data->Tab.T[i], clut_data->Params->nOutputs, saturation_factor);
                }
                #ifdef DEBUG
                else
                {  
                   printf("Adjusting color %d: ERROR, TFloat not implemented\n", ++colorNum);
                }
                #endif 
           }
        }
        clut_stage = cmsStageNext(clut_stage);
    }

    // Write the modified CLUT back to the profile
    //cmsWriteTag(hProfile, tag, lut);
}

// Function to adjust saturation for normalized float values
void AdjustSaturation(cmsCIEXYZ* xyz, double saturation_factor) {
    double gray = (xyz->X + xyz->Y + xyz->Z) / 3.0;
    xyz->X = gray + saturation_factor * (xyz->X - gray);
    xyz->Y = gray + saturation_factor * (xyz->Y - gray);
    xyz->Z = gray + saturation_factor * (xyz->Z - gray);
}

// Function to process white point (wtpt) and black point (bkpt) tags
void ProcessWhiteBlackPoints(cmsHPROFILE hProfile, double saturation_factor) {
    cmsCIEXYZ* whitePoint = (cmsCIEXYZ*)cmsReadTag(hProfile, cmsSigMediaWhitePointTag);
    if (whitePoint) {
        #ifdef DEBUG
        printf("Updating white point from %0.3f,%0.3f,%0.3f to ", whitePoint->X, whitePoint->Y, whitePoint->Z);
        #endif
        AdjustSaturation(whitePoint, saturation_factor);
        #ifdef DEBUG
        printf("%0.3f,%0.3f,%0.3f\n", whitePoint->X, whitePoint->Y, whitePoint->Z);
        #endif
        //cmsWriteTag(hProfile, cmsSigMediaWhitePointTag, whitePoint);
    } else {
        std::cerr << "White point tag not found.\n";
    }

    cmsCIEXYZ* blackPoint = (cmsCIEXYZ*)cmsReadTag(hProfile, cmsSigMediaBlackPointTag);
    if (blackPoint) {
        #ifdef DEBUG
        printf("Updating black point from %0.3f,%0.3f,%0.3f to ", blackPoint->X, blackPoint->Y, blackPoint->Z);
        #endif
        AdjustSaturation(blackPoint, saturation_factor);
        #ifdef DEBUG
        printf("%0.3f,%0.3f,%0.3f\n", blackPoint->X, blackPoint->Y, blackPoint->Z);
        #endif
        //cmsWriteTag(hProfile, cmsSigMediaBlackPointTag, blackPoint);
    } else {
        std::cerr << "Black point tag not found.\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input ICC profile> <saturation factor> <output ICC profile>\n";
        return 1;
    }

    const char* input_profile_path = argv[1];
    double saturation_factor = std::stod(argv[2]);
    const char* output_profile_path = argv[3];

    if (saturation_factor < 0.0) {
        std::cerr << "Saturation factor must be non-negative.\n";
        return 1;
    }

    // Open the ICC profile
    cmsHPROFILE hProfile = cmsOpenProfileFromFile(input_profile_path, "r");
    if (!hProfile) {
        std::cerr << "Failed to open input ICC profile.\n";
        return 1;
    }


    // Perceptal tag
    //ProcessCLUT(hProfile, cmsSigAToB0Tag, saturation_factor);

    // Process the CLUT to adjust Perceptual intent
    ProcessCLUT(hProfile, cmsSigBToA0Tag, saturation_factor);
    // Process the CLUT to adjust Media-relative colorimetric intent
    ProcessCLUT(hProfile, cmsSigBToA1Tag, saturation_factor);
    // Process the CLUT to adjust Saturation intent
    ProcessCLUT(hProfile, cmsSigBToA2Tag, saturation_factor);

    // Process the white and black points
    //ProcessWhiteBlackPoints(hProfile, saturation_factor);

    // Save the modified ICC profile
    if (!cmsSaveProfileToFile(hProfile, output_profile_path)) {
        std::cerr << "Failed to save output ICC profile.\n";
        cmsCloseProfile(hProfile);
        return 1;
    }

    // Close the profile
    cmsCloseProfile(hProfile);
    std::cout << "Saturation adjustment completed successfully.\n";
    return 0;
}

