// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "manifest.h"

using namespace bundle;

manifest_t manifest_t::read(reader_t& reader, int32_t num_files)
{
    manifest_t manifest;

    for (int32_t i = 0; i < num_files; i++)
    {
        manifest.files.emplace_back(file_entry_t::read(reader));
    }

    return manifest;
}

const file_entry_t* find(const pal::string& relative_path)
{
    for (file_entry_t &entry : files)
    {
        if (strcmp(entry.relative_path(), relative_path) == 0)
        {
            return &entry;
        }
    }
    return nullptr;
}
