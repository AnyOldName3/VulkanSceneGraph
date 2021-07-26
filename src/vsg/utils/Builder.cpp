
#include <vsg/utils/Builder.h>

using namespace vsg;

void Builder::setup(ref_ptr<Window> window, ViewportState* viewport, uint32_t maxNumTextures)
{
    auto device = window->getOrCreateDevice();

    _compile = CompileTraversal::create(window, viewport);

    // for now just allocated enough room for s
    uint32_t maxSets = maxNumTextures;
    DescriptorPoolSizes descriptorPoolSizes{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxNumTextures}};

    _compile->context.descriptorPool = DescriptorPool::create(device, maxSets, descriptorPoolSizes);

    _allocatedTextureCount = 0;
    _maxNumTextures = maxNumTextures;
}

ref_ptr<BindDescriptorSets> Builder::_createTexture(const GeometryInfo& info)
{
    auto textureData = info.image;
    if (!textureData)
    {
        if (auto itr = _colorData.find(info.color); itr != _colorData.end())
        {
            textureData = itr->second;
        }
        else
        {
            auto image = _colorData[info.color] = vec4Array2D::create(2, 2, info.color, Data::Layout{VK_FORMAT_R32G32B32A32_SFLOAT});
            image->set(0, 0, {0.0f, 1.0f, 1.0f, 1.0f});
            image->set(1, 1, {0.0f, 0.0f, 1.0f, 1.0f});
            textureData = image;
        }
    }

    auto& bindDescriptorSets = _textureDescriptorSets[textureData];
    if (bindDescriptorSets) return bindDescriptorSets;

    auto sampler = Sampler::create();

    // create texture image and associated DescriptorSets and binding
    auto texture = DescriptorImage::create(sampler, textureData, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    auto descriptorSet = DescriptorSet::create(_descriptorSetLayout, Descriptors{texture});

    bindDescriptorSets = _textureDescriptorSets[textureData] = BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, DescriptorSets{descriptorSet});
    return bindDescriptorSets;
}

ref_ptr<BindGraphicsPipeline> Builder::_createGraphicsPipeline()
{
    if (_bindGraphicsPipeline) return _bindGraphicsPipeline;

    std::cout << "Builder::_initGraphicsPipeline()" << std::endl;

    // set up search paths to SPIRV shaders and textures
    Paths searchPaths = getEnvPaths("VSG_FILE_PATH");

    ref_ptr<ShaderStage> vertexShader = ShaderStage::read(VK_SHADER_STAGE_VERTEX_BIT, "main", findFile("shaders/vert_PushConstants.spv", searchPaths));
    ref_ptr<ShaderStage> fragmentShader = ShaderStage::read(VK_SHADER_STAGE_FRAGMENT_BIT, "main", findFile("shaders/frag_PushConstants.spv", searchPaths));
    if (!vertexShader || !fragmentShader)
    {
        std::cout << "Could not create shaders." << std::endl;
        return {};
    }

    // set up graphics pipeline
    DescriptorSetLayoutBindings descriptorBindings{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
    };

    _descriptorSetLayout = DescriptorSetLayout::create(descriptorBindings);

    DescriptorSetLayouts descriptorSetLayouts{_descriptorSetLayout};

    PushConstantRanges pushConstantRanges{
        {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection view, and model matrices, actual push constant calls autoaatically provided by the VSG's DispatchTraversal
    };

    _pipelineLayout = PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

    VertexInputState::Bindings vertexBindingsDescriptions{
        VkVertexInputBindingDescription{0, sizeof(vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // vertex data
        VkVertexInputBindingDescription{1, sizeof(vec4), VK_VERTEX_INPUT_RATE_VERTEX}, // colour data
        VkVertexInputBindingDescription{2, sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX}  // tex coord data
    };

    VertexInputState::Attributes vertexAttributeDescriptions{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},    // vertex data
        VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // colour data
        VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0},       // tex coord data
    };

    auto rasteriszationState = RasterizationState::create();
    rasteriszationState->cullMode = VK_CULL_MODE_BACK_BIT;
    //rasteriszationState->cullMode = VK_CULL_MODE_NONE;

    GraphicsPipelineStates pipelineStates{
        VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
        InputAssemblyState::create(),
        rasteriszationState,
        MultisampleState::create(),
        ColorBlendState::create(),
        DepthStencilState::create()};

    auto graphicsPipeline = GraphicsPipeline::create(_pipelineLayout, ShaderStages{vertexShader, fragmentShader}, pipelineStates);
    _bindGraphicsPipeline = BindGraphicsPipeline::create(graphicsPipeline);

    return _bindGraphicsPipeline;
}

void Builder::compile(ref_ptr<Node> subgraph)
{
    if (verbose) std::cout << "Builder::compile(" << subgraph << ") _compile = " << _compile << std::endl;

    if (_compile)
    {
        subgraph->accept(*_compile);
        _compile->context.record();
        _compile->context.waitForCompletion();
    }
}

vec3 Builder::y_texcoord(const GeometryInfo& info) const
{
    if (info.image && info.image->getLayout().origin == Origin::TOP_LEFT)
    {
        return {1.0f, -1.0f, 0.0f};
    }
    else
    {
        return {0.0f, 1.0f, 1.0f};
    }
}

ref_ptr<Node> Builder::createBox(const GeometryInfo& info)
{
    auto& subgraph = _boxes[info];
    if (subgraph)
    {
        std::cout << "reused createBox()" << std::endl;
        return subgraph;
    }

    std::cout << "new createBox()" << std::endl;

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors to decorate the whole graph
    auto scenegraph = StateGroup::create();
    scenegraph->add(_createGraphicsPipeline());
    scenegraph->add(_createTexture(info));

    auto dx = info.dx;
    auto dy = info.dy;
    auto dz = info.dz;
    auto origin = info.position - dx * 0.5f - dy * 0.5f - dz * 0.5f;
    auto [t_origin, t_scale, t_top] = y_texcoord(info).value;

    vec3 v000(origin);
    vec3 v100(origin + dx);
    vec3 v110(origin + dx + dy);
    vec3 v010(origin + dy);
    vec3 v001(origin + dz);
    vec3 v101(origin + dx + dz);
    vec3 v111(origin + dx + dy + dz);
    vec3 v011(origin + dy + dz);

    // set up vertex and index arrays
    auto vertices = vec3Array::create(
        {v000, v100, v101, v001,
         v100, v110, v111, v101,
         v110, v010, v011, v111,
         v010, v000, v001, v011,
         v010, v110, v100, v000,
         v001, v101, v111, v011}); // VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_INSTANCE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

    auto colors = vec3Array::create(vertices->size(), vec3(1.0f, 1.0f, 1.0f));
    // VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

#if 0
    vec3 n0(0.0f, -1.0f, 0.0f);
    vec3 n1(1.0f, 0.0f, 0.0f);
    vec3 n2(0.0f, 1.0f, 0.0f);
    vec3 n3(0.0f, -1.0f, 0.0f);
    vec3 n4(0.0f, 0.0f, -1.0f);
    vec3 n5(0.0f, 0.0f, 1.0f);
    auto normals = vec3Array::create(
    {
        n0, n0, n0, n0,
        n1, n1, n1, n1,
        n2, n2, n2, n2,
        n3, n3, n3, n3,
        n4, n4, n4, n4,
        n5, n5, n5, n5,
    }); // VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE
#endif

    vec2 t00(0.0f, t_origin);
    vec2 t01(0.0f, t_top);
    vec2 t10(1.0f, t_origin);
    vec2 t11(1.0f, t_top);

    auto texcoords = vec2Array::create(
        {t00, t10, t11, t01,
         t00, t10, t11, t01,
         t00, t10, t11, t01,
         t00, t10, t11, t01,
         t00, t10, t11, t01,
         t00, t10, t11, t01}); // VK_FORMAT_R32G32_SFLOAT, VK_VERTEX_INPUT_RATE_VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

    auto indices = ushortArray::create(
        {0, 1, 2, 0, 2, 3,
         4, 5, 6, 4, 6, 7,
         8, 9, 10, 8, 10, 11,
         12, 13, 14, 12, 14, 15,
         16, 17, 18, 16, 18, 19,
         20, 21, 22, 20, 22, 23}); // VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

    // setup geometry
    auto vid = VertexIndexDraw::create();
    vid->arrays = DataList{vertices, colors, texcoords};
    vid->indices = indices;
    vid->indexCount = indices->size();
    vid->instanceCount = 1;

    scenegraph->addChild(vid);

    compile(scenegraph);

    subgraph = scenegraph;
    return subgraph;
}

ref_ptr<Node> Builder::createCapsule(const GeometryInfo& info)
{
    auto& subgraph = _capsules[info];
    if (subgraph)
    {
        std::cout << "reused createCapsule()" << std::endl;
        return subgraph;
    }

    std::cout << "new createCapsule()" << std::endl;

    auto [t_origin, t_scale, t_top] = y_texcoord(info).value;

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors to decorate the whole graph
    auto scenegraph = StateGroup::create();
    scenegraph->add(_createGraphicsPipeline());
    scenegraph->add(_createTexture(info));

    auto dx = info.dx * 0.5f;
    auto dy = info.dy * 0.5f;
    auto dz = info.dz * 0.5f;

    auto bottom = info.position - dz;
    auto top = info.position + dz;

    bool withEnds = true;

    unsigned int num_columns = 20;
    unsigned int num_rows = 6;

    unsigned int num_vertices = num_columns * 2;
    unsigned int num_indices = (num_columns - 1) * 6;

    if (withEnds)
    {
        num_vertices += num_columns * num_rows * 2;
        num_indices += (num_columns - 1) * (num_rows - 1) * 6 * 2;
    }

    auto vertices = vec3Array::create(num_vertices);
    auto normals = vec3Array::create(num_vertices);
    auto texcoords = vec2Array::create(num_vertices);
    auto colors = vec3Array::create(vertices->size(), vec3(1.0f, 1.0f, 1.0f));
    auto indices = ushortArray::create(num_indices);

    vec3 v = dy;
    vec3 n = normalize(dy);
    vertices->set(0, bottom + v);
    normals->set(0, n);
    texcoords->set(0, vec2(0.0, t_origin));
    vertices->set(num_columns * 2 - 2, bottom + v);
    normals->set(num_columns * 2 - 2, n);
    texcoords->set(num_columns * 2 - 2, vec2(1.0, t_origin));

    vertices->set(1, top + v);
    normals->set(1, n);
    texcoords->set(1, vec2(0.0, t_top));
    vertices->set(num_columns * 2 - 1, top + v);
    normals->set(num_columns * 2 - 1, n);
    texcoords->set(num_columns * 2 - 1, vec2(1.0, t_top));

    for (unsigned int c = 1; c < num_columns - 1; ++c)
    {
        unsigned int vi = c * 2;
        float r = float(c) / float(num_columns - 1);
        float alpha = (r)*2.0 * PI;
        v = dx * (-sinf(alpha)) + dy * (cosf(alpha));
        n = normalize(v);

        vertices->set(vi, bottom + v);
        normals->set(vi, n);
        texcoords->set(vi, vec2(r, t_origin));

        vertices->set(vi + 1, top + v);
        normals->set(vi + 1, n);
        texcoords->set(vi + 1, vec2(r, t_top));
    }

    unsigned int i = 0;
    for (unsigned int c = 0; c < num_columns - 1; ++c)
    {
        unsigned lower = c * 2;
        unsigned upper = lower + 1;

        indices->set(i++, lower);
        indices->set(i++, lower + 2);
        indices->set(i++, upper);

        indices->set(i++, upper);
        indices->set(i++, lower + 2);
        indices->set(i++, upper + 2);
    }

    if (withEnds)
    {
        unsigned int base_vi = num_columns * 2;

        // bottom
        {
            for (unsigned int r = 0; r < num_rows; ++r)
            {
                float beta = ((float(r) / float(num_rows - 1)) - 1.0f) * PI * 0.5f;
                float ty = t_origin + t_scale * float(r) / float(num_rows - 1);
                float cos_beta = cosf(beta);
                vec3 dz_sin_beta = dz * sinf(beta);

                v = dy * cos_beta + dz_sin_beta;
                n = normalize(v);

                unsigned int left_i = base_vi + r * num_columns;
                vertices->set(left_i, bottom + v);
                normals->set(left_i, n);
                texcoords->set(left_i, vec2(0.0f, ty));

                unsigned int right_i = left_i + num_columns - 1;
                vertices->set(right_i, bottom + v);
                normals->set(right_i, n);
                texcoords->set(right_i, vec2(1.0f, ty));

                for (unsigned int c = 1; c < num_columns - 1; ++c)
                {
                    unsigned int vi = left_i + c;
                    float alpha = (float(c) / float(num_columns - 1)) * 2.0 * PI;
                    v = dx * (-sinf(alpha) * cos_beta) + dy * (cosf(alpha) * cos_beta) + dz_sin_beta;
                    n = normalize(v);
                    vertices->set(vi, bottom + v);
                    normals->set(vi, n);
                    texcoords->set(vi, vec2(float(c) / float(num_columns - 1), ty));
                }
            }

            for (unsigned int r = 0; r < num_rows - 1; ++r)
            {
                for (unsigned int c = 0; c < num_columns - 1; ++c)
                {
                    unsigned lower = base_vi + num_columns * r + c;
                    unsigned upper = lower + num_columns;

                    indices->set(i++, lower);
                    indices->set(i++, lower + 1);
                    indices->set(i++, upper);

                    indices->set(i++, upper);
                    indices->set(i++, lower + 1);
                    indices->set(i++, upper + 1);
                }
            }

            base_vi += num_columns * num_rows;
        }

        // top
        {
            for (unsigned int r = 0; r < num_rows; ++r)
            {
                float beta = ((float(r) / float(num_rows - 1))) * PI * 0.5f;
                float ty = t_origin + t_scale * float(r) / float(num_rows - 1);
                float cos_beta = cosf(beta);
                vec3 dz_sin_beta = dz * sinf(beta);

                v = dy * cos_beta + dz_sin_beta;
                n = normalize(v);

                unsigned int left_i = base_vi + r * num_columns;
                vertices->set(left_i, top + v);
                normals->set(left_i, n);
                texcoords->set(left_i, vec2(0.0f, ty));

                unsigned int right_i = left_i + num_columns - 1;
                vertices->set(right_i, top + v);
                normals->set(right_i, n);
                texcoords->set(right_i, vec2(1.0f, ty));

                for (unsigned int c = 1; c < num_columns - 1; ++c)
                {
                    unsigned int vi = left_i + c;
                    float alpha = (float(c) / float(num_columns - 1)) * 2.0 * PI;
                    v = dx * (-sinf(alpha) * cos_beta) + dy * (cosf(alpha) * cos_beta) + dz_sin_beta;
                    n = normalize(v);
                    vertices->set(vi, top + v);
                    normals->set(vi, n);
                    texcoords->set(vi, vec2(float(c) / float(num_columns - 1), ty));
                }
            }

            for (unsigned int r = 0; r < num_rows - 1; ++r)
            {
                for (unsigned int c = 0; c < num_columns - 1; ++c)
                {
                    unsigned lower = base_vi + num_columns * r + c;
                    unsigned upper = lower + num_columns;

                    indices->set(i++, lower);
                    indices->set(i++, lower + 1);
                    indices->set(i++, upper);

                    indices->set(i++, upper);
                    indices->set(i++, lower + 1);
                    indices->set(i++, upper + 1);
                }
            }

            base_vi += num_columns * num_rows;
        }
    }

    // setup geometry
    auto vid = VertexIndexDraw::create();
    vid->arrays = DataList{vertices, colors, texcoords};
    vid->indices = indices;
    vid->indexCount = indices->size();
    vid->instanceCount = 1;

    scenegraph->addChild(vid);

    compile(scenegraph);

    subgraph = scenegraph;
    return subgraph;
}

ref_ptr<Node> Builder::createCone(const GeometryInfo& info)
{
    auto& subgraph = _cones[info];
    if (subgraph)
    {
        std::cout << "reused createCone()" << std::endl;
        return subgraph;
    }

    std::cout << "new createCone()" << std::endl;

    auto [t_origin, t_scale, t_top] = y_texcoord(info).value;

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors to decorate the whole graph
    auto scenegraph = StateGroup::create();
    scenegraph->add(_createGraphicsPipeline());
    scenegraph->add(_createTexture(info));

    auto dx = info.dx * 0.5f;
    auto dy = info.dy * 0.5f;
    auto dz = info.dz * 0.5f;

    auto bottom = info.position - dz;
    auto top = info.position + dz;

    bool withEnds = false;

    unsigned int num_columns = 20;
    unsigned int num_vertices = num_columns * 2;
    unsigned int num_indices = (num_columns - 1) * 3;

    if (withEnds)
    {
        num_vertices += num_columns;
        num_indices += (num_columns - 2) * 3;
    }

    auto vertices = vec3Array::create(num_vertices);
    auto normals = vec3Array::create(num_vertices);
    auto texcoords = vec2Array::create(num_vertices);
    auto colors = vec3Array::create(vertices->size(), vec3(1.0f, 1.0f, 1.0f));
    auto indices = ushortArray::create(num_indices);

    vec3 v = dy;
    vec3 n = normalize(dy);
    vertices->set(0, bottom + v);
    normals->set(0, n);
    texcoords->set(0, vec2(0.0, t_origin));
    vertices->set(num_columns * 2 - 2, bottom + v);
    normals->set(num_columns * 2 - 2, n);
    texcoords->set(num_columns * 2 - 2, vec2(1.0, t_origin));

    vertices->set(1, top);
    normals->set(1, n);
    texcoords->set(1, vec2(0.0, t_top));
    vertices->set(num_columns * 2 - 1, top);
    normals->set(num_columns * 2 - 1, n);
    texcoords->set(num_columns * 2 - 1, vec2(1.0, t_top));

    for (unsigned int c = 1; c < num_columns - 1; ++c)
    {
        unsigned int vi = c * 2;
        float r = float(c) / float(num_columns - 1);
        float alpha = (r)*2.0 * PI;
        v = dx * (-sinf(alpha)) + dy * (cosf(alpha));

        float alpha_neighbour = alpha + 0.01;
        vec3 v_neighour = dx * (-sinf(alpha_neighbour)) + dy * (cosf(alpha_neighbour));

        n = normalize(cross(v - top, v_neighour - v));

        vertices->set(vi, bottom + v);
        normals->set(vi, n);
        texcoords->set(vi, vec2(r, t_origin));

        vertices->set(vi + 1, top);
        normals->set(vi + 1, n);
        texcoords->set(vi + 1, vec2(r, t_top));
    }

    unsigned int i = 0;
    for (unsigned int c = 0; c < num_columns - 1; ++c)
    {
        unsigned lower = c * 2;
        unsigned upper = lower + 1;

        indices->set(i++, lower);
        indices->set(i++, lower + 2);
        indices->set(i++, upper);
    }

    if (withEnds)
    {
        unsigned int bottom_i = num_columns * 2;
        v = dy;
        vec3 bottom_n = normalize(-dz);

        vertices->set(bottom_i, bottom + v);
        normals->set(bottom_i, bottom_n);
        texcoords->set(bottom_i, vec2(0.0, t_origin));
        vertices->set(bottom_i + num_columns - 1, bottom + v);
        normals->set(bottom_i + num_columns - 1, bottom_n);
        texcoords->set(bottom_i + num_columns - 1, vec2(1.0, t_origin));

        for (unsigned int c = 1; c < num_columns - 1; ++c)
        {
            float r = float(c) / float(num_columns - 1);
            float alpha = (r)*2.0 * PI;
            v = dx * (-sinf(alpha)) + dy * (cosf(alpha));

            unsigned int vi = bottom_i + c;
            vertices->set(vi, bottom + v);
            normals->set(vi, bottom_n);
            texcoords->set(vi, vec2(r, t_origin));
        }

        for (unsigned int c = 0; c < num_columns - 2; ++c)
        {
            indices->set(i++, bottom_i + c);
            indices->set(i++, bottom_i + num_columns - 1);
            indices->set(i++, bottom_i + c + 1);
        }
    }

    // setup geometry
    auto vid = VertexIndexDraw::create();
    vid->arrays = DataList{vertices, colors, texcoords};
    vid->indices = indices;
    vid->indexCount = indices->size();
    vid->instanceCount = 1;

    scenegraph->addChild(vid);

    compile(scenegraph);

    subgraph = scenegraph;
    return subgraph;
}

ref_ptr<Node> Builder::createCylinder(const GeometryInfo& info)
{
    auto& subgraph = _cylinders[info];
    if (subgraph)
    {
        std::cout << "reused createCylinder()" << std::endl;
        return subgraph;
    }

    std::cout << "new createCylinder()" << std::endl;

    auto [t_origin, t_scale, t_top] = y_texcoord(info).value;

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors to decorate the whole graph
    auto scenegraph = StateGroup::create();
    scenegraph->add(_createGraphicsPipeline());
    scenegraph->add(_createTexture(info));

    auto dx = info.dx * 0.5f;
    auto dy = info.dy * 0.5f;
    auto dz = info.dz * 0.5f;

    auto bottom = info.position - dz;
    auto top = info.position + dz;

    bool withEnds = true;

    unsigned int num_columns = 20;
    unsigned int num_vertices = num_columns * 2;
    unsigned int num_indices = (num_columns - 1) * 6;

    if (withEnds)
    {
        num_vertices += num_columns * 2;
        num_indices += (num_columns - 2) * 6;
    }

    auto vertices = vec3Array::create(num_vertices);
    auto normals = vec3Array::create(num_vertices);
    auto texcoords = vec2Array::create(num_vertices);
    auto colors = vec3Array::create(vertices->size(), vec3(1.0f, 1.0f, 1.0f));
    auto indices = ushortArray::create(num_indices);

    vec3 v = dy;
    vec3 n = normalize(dy);
    vertices->set(0, bottom + v);
    normals->set(0, n);
    texcoords->set(0, vec2(0.0, t_origin));
    vertices->set(num_columns * 2 - 2, bottom + v);
    normals->set(num_columns * 2 - 2, n);
    texcoords->set(num_columns * 2 - 2, vec2(1.0, t_origin));

    vertices->set(1, top + v);
    normals->set(1, n);
    texcoords->set(1, vec2(0.0, t_top));
    vertices->set(num_columns * 2 - 1, top + v);
    normals->set(num_columns * 2 - 1, n);
    texcoords->set(num_columns * 2 - 1, vec2(1.0, t_top));

    for (unsigned int c = 1; c < num_columns - 1; ++c)
    {
        unsigned int vi = c * 2;
        float r = float(c) / float(num_columns - 1);
        float alpha = (r)*2.0 * PI;
        v = dx * (-sinf(alpha)) + dy * (cosf(alpha));
        n = normalize(v);

        vertices->set(vi, bottom + v);
        normals->set(vi, n);
        texcoords->set(vi, vec2(r, t_origin));

        vertices->set(vi + 1, top + v);
        normals->set(vi + 1, n);
        texcoords->set(vi + 1, vec2(r, t_top));
    }

    unsigned int i = 0;
    for (unsigned int c = 0; c < num_columns - 1; ++c)
    {
        unsigned lower = c * 2;
        unsigned upper = lower + 1;

        indices->set(i++, lower);
        indices->set(i++, lower + 2);
        indices->set(i++, upper);

        indices->set(i++, upper);
        indices->set(i++, lower + 2);
        indices->set(i++, upper + 2);
    }

    if (withEnds)
    {
        unsigned int bottom_i = num_columns * 2;
        v = dy;
        vec3 bottom_n = normalize(-dz);
        vertices->set(bottom_i, bottom + v);
        normals->set(bottom_i, n);
        texcoords->set(bottom_i, vec2(0.0, t_origin));
        vertices->set(bottom_i + num_columns - 1, bottom + v);
        normals->set(bottom_i + num_columns - 1, n);
        texcoords->set(bottom_i + num_columns - 1, vec2(1.0, t_origin));

        unsigned int top_i = bottom_i + num_columns;
        vec3 top_n = normalize(dz);
        vertices->set(top_i, top + v);
        normals->set(top_i, n);
        texcoords->set(top_i, vec2(0.0, t_top));
        vertices->set(top_i + num_columns - 1, top + v);
        normals->set(top_i + num_columns - 1, n);
        texcoords->set(top_i + num_columns - 1, vec2(0.0, t_top));

        for (unsigned int c = 1; c < num_columns - 1; ++c)
        {
            float r = float(c) / float(num_columns - 1);
            float alpha = (r)*2.0 * PI;
            v = dx * (-sinf(alpha)) + dy * (cosf(alpha));
            n = normalize(v);

            unsigned int vi = bottom_i + c;
            vertices->set(vi, bottom + v);
            normals->set(vi, bottom_n);
            texcoords->set(vi, vec2(r, t_origin));

            vi = top_i + c;
            vertices->set(vi, top + v);
            normals->set(vi, top_n);
            texcoords->set(vi, vec2(r, t_top));
        }

        for (unsigned int c = 0; c < num_columns - 2; ++c)
        {
            indices->set(i++, bottom_i + c);
            indices->set(i++, bottom_i + num_columns - 1);
            indices->set(i++, bottom_i + c + 1);
        }

        for (unsigned int c = 0; c < num_columns - 2; ++c)
        {
            indices->set(i++, top_i + c);
            indices->set(i++, top_i + c + 1);
            indices->set(i++, top_i + num_columns - 1);
        }
    }

    // setup geometry
    auto vid = VertexIndexDraw::create();
    vid->arrays = DataList{vertices, colors, texcoords};
    vid->indices = indices;
    vid->indexCount = indices->size();
    vid->instanceCount = 1;

    scenegraph->addChild(vid);

    compile(scenegraph);

    subgraph = scenegraph;
    return subgraph;
}

ref_ptr<Node> Builder::createQuad(const GeometryInfo& info)
{
    auto& subgraph = _boxes[info];
    if (subgraph)
    {
        std::cout << "reused createQuad()" << std::endl;
        return subgraph;
    }

    std::cout << "new createQuad()" << std::endl;

    auto scenegraph = StateGroup::create();
    scenegraph->add(_createGraphicsPipeline());
    scenegraph->add(_createTexture(info));

    auto dx = info.dx;
    auto dy = info.dy;
    auto origin = info.position - dx * 0.5f - dy * 0.5f;
    auto [t_origin, t_scale, t_top] = y_texcoord(info).value;

    // set up vertex and index arrays
    auto vertices = vec3Array::create(
        {origin,
         origin + dx,
         origin + dx + dy,
         origin + dy}); // VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_INSTANCE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

    auto colors = vec3Array::create(
        {{1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 1.0f, 1.0f}}); // VK_FORMAT_R32G32B32_SFLOAT, VK_VERTEX_INPUT_RATE_VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

    auto texcoords = vec2Array::create(
        {{0.0f, t_origin},
         {1.0f, t_origin},
         {1.0f, t_top},
         {0.0f, t_top}}); // VK_FORMAT_R32G32_SFLOAT, VK_VERTEX_INPUT_RATE_VERTEX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

    auto indices = ushortArray::create(
        {
            0,
            1,
            2,
            2,
            3,
            0,
        }); // VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE

    // setup geometry
    auto vid = VertexIndexDraw::create();
    vid->arrays = DataList{vertices, colors, texcoords};
    vid->indices = indices;
    vid->indexCount = indices->size();
    vid->instanceCount = 1;

    scenegraph->addChild(vid);

    compile(scenegraph);

    return scenegraph;
}

ref_ptr<Node> Builder::createSphere(const GeometryInfo& info)
{
    auto& subgraph = _spheres[info];
    if (subgraph)
    {
        std::cout << "reused createSphere()" << std::endl;
        return subgraph;
    }

    std::cout << "new createSphere()" << std::endl;

    auto [t_origin, t_scale, t_top] = y_texcoord(info).value;

    // create StateGroup as the root of the scene/command graph to hold the GraphicsProgram, and binding of Descriptors to decorate the whole graph
    auto scenegraph = StateGroup::create();
    scenegraph->add(_createGraphicsPipeline());
    scenegraph->add(_createTexture(info));

    auto dx = info.dx * 0.5f;
    auto dy = info.dy * 0.5f;
    auto dz = info.dz * 0.5f;
    auto origin = info.position;

    unsigned int num_columns = 20;
    unsigned int num_rows = 10;
    unsigned int num_vertices = num_columns * num_rows;
    unsigned int num_indices = (num_columns - 1) * (num_rows - 1) * 6;

    auto vertices = vec3Array::create(num_vertices);
    auto normals = vec3Array::create(num_vertices);
    auto texcoords = vec2Array::create(num_vertices);
    auto colors = vec3Array::create(vertices->size(), vec3(1.0f, 1.0f, 1.0f));
    auto indices = ushortArray::create(num_indices);

    for (unsigned int r = 0; r < num_rows; ++r)
    {
        float beta = ((float(r) / float(num_rows - 1)) - 0.5) * PI;
        float ty = t_origin + t_scale * float(r) / float(num_rows - 1);
        float cos_beta = cosf(beta);
        vec3 dz_sin_beta = dz * sinf(beta);

        vec3 v = dy * cos_beta + dz_sin_beta;
        vec3 n = normalize(v);

        unsigned int left_i = r * num_columns;
        vertices->set(left_i, v + origin);
        normals->set(left_i, n);
        texcoords->set(left_i, vec2(0.0f, ty));

        unsigned int right_i = left_i + num_columns - 1;
        vertices->set(right_i, v + origin);
        normals->set(right_i, n);
        texcoords->set(right_i, vec2(1.0f, ty));

        for (unsigned int c = 1; c < num_columns - 1; ++c)
        {
            unsigned int vi = left_i + c;
            float alpha = (float(c) / float(num_columns - 1)) * 2.0 * PI;
            v = dx * (-sinf(alpha) * cos_beta) + dy * (cosf(alpha) * cos_beta) + dz_sin_beta;
            n = normalize(v);
            vertices->set(vi, origin + v);
            normals->set(vi, n);
            texcoords->set(vi, vec2(float(c) / float(num_columns - 1), ty));
        }
    }

    unsigned int i = 0;
    for (unsigned int r = 0; r < num_rows - 1; ++r)
    {
        for (unsigned int c = 0; c < num_columns - 1; ++c)
        {
            unsigned lower = num_columns * r + c;
            unsigned upper = lower + num_columns;

            indices->set(i++, lower);
            indices->set(i++, lower + 1);
            indices->set(i++, upper);

            indices->set(i++, upper);
            indices->set(i++, lower + 1);
            indices->set(i++, upper + 1);
        }
    }

    // setup geometry
    auto vid = VertexIndexDraw::create();
    vid->arrays = DataList{vertices, colors, texcoords};
    vid->indices = indices;
    vid->indexCount = indices->size();
    vid->instanceCount = 1;

    scenegraph->addChild(vid);

    compile(scenegraph);

    subgraph = scenegraph;
    return subgraph;
}
