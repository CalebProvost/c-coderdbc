#include "dbcscanner.h"
#include <algorithm>
#include <math.h>

#define MAX_LINE 256

char line[MAX_LINE] = { 0 };

MessageDescriptor_t* find_message(vector<MessageDescriptor_t*> msgs, uint32_t ID)
{
  MessageDescriptor_t* ret = nullptr;

  if (msgs.size() == 0)
    return ret;

  for (size_t i = 0; i < msgs.size(); i++)
  {
    ret = msgs[i];

    if (ret->MsgID == ID)
      // Frame found
      break;
  }

  return ret;
}

DbcScanner::DbcScanner()
{
}

DbcScanner::~DbcScanner()
{
}

int32_t DbcScanner::TrimDbcText(istream& readstrm)
{
  msgs.clear();

  // Search each message and signals in dbc source data,
  // and fill @msgs collection
  ParseMessageInfo(readstrm);

  // all the messages and signals were read from source
  // scan it one more time to define all attributes and
  // more additional information
  readstrm.clear();
  readstrm.seekg(0);

  // Search all attributes and additional information
  // and update specific fields in messages
  ParseOtherInfo(readstrm);

  return 0;
}


void DbcScanner::ParseMessageInfo(istream& readstrm)
{
  std::string sline;

  MessageDescriptor_t* pMsg = nullptr;

  while (readstrm.eof() == false)
  {
    readstrm.getline(line, MAX_LINE);

    sline = line;

    // New message line has been found
    if (lparser.IsMessageLine(sline))
    {
      // check if the pMsg takes previous message
      AddMessage(pMsg);

      // create instance for the detected message
      pMsg = new MessageDescriptor_t;

      if (!lparser.ParseMessageLine(pMsg, sline))
      {
        // the message has invalid format so drop it and wait next one
        delete pMsg;
        pMsg = nullptr;
      }
    }

    if (pMsg != nullptr && lparser.IsSignalLine(sline))
    {
      SignalDescriptor_t sig;

      // parse signal line
      if (lparser.ParseSignalLine(&sig, sline))
      {
        // put successfully parsed  signal to the message signals
        pMsg->Signals.push_back(sig);
      }
    }
  }

  // check if the pMsg takes previous message
  AddMessage(pMsg);
}


void DbcScanner::ParseOtherInfo(istream& readstrm)
{
  std::string sline;

  Comment_t cmmnt;

  AttributeDescriptor_t attr;

  while (!readstrm.eof())
  {
    readstrm.getline(line, MAX_LINE);

    sline = line;

    if (lparser.ParseCommentLine(&cmmnt, sline))
    {
      uint32_t found_msg_id = cmmnt.MsgId;

      // update message comment field
      auto msg = find_message(msgs, cmmnt.MsgId);

      if (msg != nullptr)
      {
        // comment line was found
        if (cmmnt.ca_target == CommentTarget::Message)
        {
          // put comment to message descriptor
          msg->CommentText = cmmnt.Text;
        }
        else if (cmmnt.ca_target == CommentTarget::Signal)
        {
          for (size_t i = 0; i < msg->Signals.size(); i++)
          {
            if (cmmnt.SigName == msg->Signals[i].Name)
            {
              // signal has been found, update commnet text
              msg->Signals[i].CommentText = cmmnt.Text;
            }
          }
        }
      }
    }

    if (lparser.ParseValTableLine(&cmmnt, sline))
    {
      uint32_t found_msg_id = cmmnt.MsgId;

      // update message comment field
      auto msg = find_message(msgs, cmmnt.MsgId);

      if (msg != nullptr)
      {
        // comment line was found
        if (cmmnt.ca_target == CommentTarget::Message)
        {
          // put comment to message descriptor
          msg->CommentText = cmmnt.Text;
        }
        else if (cmmnt.ca_target == CommentTarget::Signal)
        {
          for (size_t i = 0; i < msg->Signals.size(); i++)
          {
            if (cmmnt.SigName == msg->Signals[i].Name)
            {
              // signal has been found, update commnet text
              msg->Signals[i].ValueText = cmmnt.Text;
            }
          }
        }
      }
    }

    if (lparser.ParseAttributeLine(&attr, sline))
    {
      auto msg = find_message(msgs, attr.MsgId);

      if (msg != nullptr)
      {
        // message was found, set attribute value
        if (attr.Type == AttributeType::CycleTime)
        {
          msg->Cycle = attr.Value;
        }
      }
    }
  }
}


void DbcScanner::AddMessage(MessageDescriptor_t* message)
{
  if (message != nullptr)
  {
    if (message->Signals.size() > 0)
    {
      // sort signals by start bit
      std::sort(message->Signals.begin(), message->Signals.end(),
        [](const SignalDescriptor_t& a, const SignalDescriptor_t& b) -> bool
        {
          return a.StartBit < b.StartBit;
        });

      for (size_t i = 0; i < message->Signals.size(); i++)
      {
        for (size_t j = 0; j < message->Signals[i].RecS.size(); j++)
        {
          string val = message->Signals[i].RecS[j];

          if (std::find(message->RecS.begin(), message->RecS.end(), val) == message->RecS.end())
          {
            // add another one RX node
            message->RecS.push_back(val);
          }
        }
      }
    }

    // save pointer on message
    msgs.push_back(message);
  }
}


void DbcScanner::SetDefualtMessage(MessageDescriptor_t* message)
{
  message->CommentText = "";
  message->Cycle = 0;
  message->DLC = 0;
  message->IsExt = false;
  message->MsgID = 0;
  message->Name = "";
  message->RecS.clear();
  message->Signals.clear();
  message->Transmitter = "";
}