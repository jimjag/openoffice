/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/
#ifndef GCONXRM_HXX
#define GCONXRM_HXX
#include "gLang.hxx"



/*****************************************************************************
 *************************   G C O N X R M . H X X   *************************
 *****************************************************************************
 * This is the class definition header xrm converter
 *****************************************************************************/



/********************   C L A S S   D E F I N I T I O N   ********************/
class convert_xrm_impl : public convert_gen
{
  public:
    convert_xrm_impl(const std::string& srSourceFile, l10nMem& crMemory);
    ~convert_xrm_impl();

    void startCollectData(std::string sType, std::string& sCollectedText);
    void stopCollectData(std::string sType, std::string& sCollectedText);
    void collectData(std::string& sCollectedText);

  private:
    bool   mbCollectingData;
    std::string msCollector;
	std::string msTag;
	std::string msMergeType;

    void extract();
    void insert();
    void runLex();

    friend class convert_xrm;
};
#endif
