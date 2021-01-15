////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 Ripose
//
// This file is part of Memento.
//
// Memento is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2 of the License.
//
// Memento is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Memento.  If not, see <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include "directoryutils.h"

#define DICTIONARY_DIR "dict"
#define CACHE_DIR "dict"

QString DirectoryUtils::getConfigDir()
{
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        WCHAR buf[MAX_PATH];
        GetModuleFileNameW(NULL, buf, MAX_PATH);
        QString path = QString::fromWCharArray(buf);
        path = path.left(path.lastIndexOf(SLASH) + 1);
    #else
        QString path = BASE_DIR;
    #endif
    path += CONFIG_PATH;
    path += SLASH;
    return path;
}

QString DirectoryUtils::getDictionaryDir()
{
    return getConfigDir() + DICTIONARY_DIR + SLASH;
}

QString DirectoryUtils::getCacheDir()
{
    return getConfigDir() + CACHE_DIR + SLASH;
}