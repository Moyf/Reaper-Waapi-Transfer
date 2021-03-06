#include "WAAPIHelpers.h"
#include "Reaper_WAAPI_Transfer.h"

std::unordered_map<std::string, int> WwiseImageList::iconList = std::unordered_map<std::string, int>{};
HIMAGELIST WwiseImageList::imageList = HIMAGELIST{};

void WwiseImageList::LoadIcons(std::initializer_list<std::pair<std::string, int>> icons)
{
    if (imageList)
    {
        ImageList_Destroy(imageList);
    }

    imageList = ImageList_Create(15, 15, ILC_COLOR32, static_cast<int>(icons.size()), 0);

    for (const auto &icon : icons)
    {
        HANDLE imgHandle = LoadImage(g_hInst, MAKEINTRESOURCE(icon.second), IMAGE_ICON, 0, 0, 0);
        if (imgHandle)
        {
            int imageId = ImageList_AddIcon(imageList, (HICON)imgHandle);
            if (imageId != -1)
            {
                iconList.insert({ icon.first, imageId });
            }
        }
    }
}

int WwiseImageList::GetIconForWwiseType(const std::string wwiseType)
{
    auto foundIcon = iconList.find(wwiseType);
    if (foundIcon != iconList.end())
    {
        return foundIcon->second;
    }
    else
    {
        return -1;
    }
}


bool GetAllSelectedWwiseObjects(AK::WwiseAuthoringAPI::AkJson &resultsOut, 
                                AK::WwiseAuthoringAPI::Client &client, 
                                bool GetNotes)
{
    using namespace AK::WwiseAuthoringAPI;
    AkJson resultsJson;
    
    AkJson options(AkJson::Map{
        { "return", AkJson::Array{ 
        AkVariant("id"), 
        AkVariant("name"), 
        AkVariant("type"), 
        AkVariant("path"), 
        AkVariant("parent"), 
        AkVariant("childrenCount"), } }
    });

    if (GetNotes)
    {
        options["return"].GetArray().push_back(AkVariant("notes"));
    }


    return client.Call(ak::wwise::ui::getSelectedObjects, AkJson(AkJson::Map()), options, resultsOut);
}

void GetWaapiResultsArray(AK::WwiseAuthoringAPI::AkJson::Array &arrayIn,
                          AK::WwiseAuthoringAPI::AkJson &results)
{
    using namespace AK::WwiseAuthoringAPI;
    switch (results.GetType())
    {
    case AkJson::Type::Map:
    {
        if (results.HasKey("objects"))
        {
            arrayIn = results["objects"].GetArray();
            return;
        }
        else if (results.HasKey("return"))
        {
            arrayIn = results["return"].GetArray();
            return;
        }
        else
        {
            assert(!"Not implemented.");
            return;
        }

    } break;


    default:
        assert(!"Not implemented.");
        return;
    }
}

bool GetChildren(const AK::WwiseAuthoringAPI::AkVariant &path, 
                 AK::WwiseAuthoringAPI::AkJson &results, 
                 AK::WwiseAuthoringAPI::Client &client,
                 bool getNotes)
{
    using namespace AK::WwiseAuthoringAPI;
    AkJson query;
    AkJson args(AkJson::Map{
        { "from", AkJson::Map{
            { "path", AkJson::Array{ path } } } },
            { "transform",
            { AkJson::Array{ AkJson::Map{ { "select", AkJson::Array{ { "children" } } } } } }
        }
    });

    AkJson options(AkJson::Map{
        { "return", AkJson::Array{
        AkVariant("id"), 
        AkVariant("name"), 
        AkVariant("path"), 
        AkVariant("type"), 
        AkVariant("parent"),
        AkVariant("childrenCount") 
    }}
    });

    if (getNotes)
    {
        options["return"].GetArray().push_back(AkVariant("notes"));
    }

    return client.Call(ak::wwise::core::object::get, args, options, results);
}

bool WaapiImportItems(const AK::WwiseAuthoringAPI::AkJson::Array &items, 
                      AK::WwiseAuthoringAPI::Client &client, 
                      WAAPIImportOperation importOperation)
{
    using namespace AK::WwiseAuthoringAPI;

    AkJson createArgs(AkJson::Map{
        { "importOperation", AkVariant(GetImportOperationString(importOperation)) },
        { "default", AkJson::Map{
            { "importLanguage", AkVariant("SFX") }
    } },
    { "imports", items }
    });
    AkJson result;

    return client.Call(ak::wwise::core::audio::import, createArgs, AkJson(AkJson::Map()), result);
}
