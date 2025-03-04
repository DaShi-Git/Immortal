#pragma once

#include "Core.h"

#include "Texture.h"

namespace Immortal
{

class IMMORTAL_API Environment
{
public:
    /* Specular Bidirectional Reflectance Distribution Function Look up table */
    std::shared_ptr<Texture> SpecularBRDFLookUpTable;

    std::shared_ptr<TextureCube> IrradianceMap;

    Environment(std::shared_ptr<TextureCube> &skyboxTexture);

    ~Environment();
};

}

