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
#include "gConHrc.hxx"



/*****************************************************************************
 *********************   G C O N H R C W R A P . C X X   *********************
 *****************************************************************************
 * This includes the c code generated by flex
 *****************************************************************************/






/**********************   I M P L E M E N T A T I O N   **********************/
namespace HrcWrap
{
#include "gConHrc_yy.c"
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_hrc_impl::runLex()
{
  HrcWrap::genHrc_lex();
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_hrc_impl::setKey(std::string &sText)
{
  int    nL, nE;

  // write text for merge
  if (mbMergeMode)
    writeSourceFile(msCollector + sText);
  msCollector.clear();

  // locate id
  for (nL = 0; sText[nL] != ' ' && sText[nL] != '\t' && sText[nL] != '\n'; ++nL) ;
  for (; sText[nL] == ' ' || sText[nL] == '\t'; ++nL) ;
  for (nE = nL; sText[nE] != ' ' && sText[nE] != '\t' && sText[nE] != '\n'; ++nE) ;
  msKey = sText.substr(nL, nE - nL);
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_hrc_impl::saveData(std::string &sText)
{
  int    nL, nE;
  std::string sUseText;

  // write text for merge
  if (mbMergeMode)
    writeSourceFile(msCollector + sText);
  msCollector.clear();

  // locate id
  for (nL = 0; sText[nL] != '=' && sText[nL] != '\n'; ++nL) ;
  for (; sText[nL] != '\"'; ++nL) ;
  for (nE = nL+1; sText[nE] != '\"'; ++nE) ;
  sUseText = sText.substr(nL+1, nE - nL -1);

  if (mbMergeMode)
  {
    // get all languages (includes en-US)
    std::vector<l10nMem_entry *>& cExtraLangauges = mcMemory.getLanguagesForKey(msKey);
    std::string                   sNewLine;
    int                      nL = cExtraLangauges.size();

    for (int i = 0; i < nL; ++i)
    {
      sNewLine = "<value xml:lang=\"" + cExtraLangauges[i]->msLanguage + "\">" +
	             cExtraLangauges[i]->msText + "</value>";
      writeSourceFile(sNewLine);
    }
  }
  else
    mcMemory.setEnUsKey(msKey, sUseText);
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_hrc_impl::copyData(std::string &sText)
{
  msCollector += sText;
  if (sText == "\n")
  {
    if (mbMergeMode)
      writeSourceFile(msCollector);
    msCollector.clear();
  }
}
