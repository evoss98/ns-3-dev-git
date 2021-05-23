
//implementation of drr queue for NS-3
//Ang Li and Erin Voss
//parts of the code are adopted from the fq-codel-queue-disc that
//is already implemented in NS3

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/queue.h"
#include "drr-queue-disc.h"
#include "ns3/net-device-queue-interface.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE ("DrrQueueDisc");
    NS_OBJECT_ENSURE_REGISTERED (DrrFlow);

    //define DrrFlow
    TypeId DrrFlow::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::DrrFlow")
            .SetParent<QueueDiscClass> ()
            .SetGroupName ("TrafficControl")
            .AddConstructor<DrrFlow> ()
        ;
        return tid;
    }  

    //constuctor and deconstructor. Adapted from fq-codel-queue
    DrrFlow::DrrFlow ()
        : m_deficit (0),
        m_status (INACTIVE)
    {
    }

    DrrFlow::~DrrFlow ()
    {
    } 

    //SetDeficit, GetDeficit, IncreaseDeficit, SetStatus, FlowStatus taken from fq-codel-queue
    void
    DrrFlow::SetDeficit (uint32_t deficit)
    {
        NS_LOG_FUNCTION (this << deficit);
        m_deficit = deficit;
    }

    int32_t
    DrrFlow::GetDeficit (void) const
    {
        return m_deficit;
    }

    void
    DrrFlow::IncreaseDeficit (int32_t deficit)
    {
        NS_LOG_FUNCTION (this << deficit);
        m_deficit += deficit;
    }

    void
    DrrFlow::DecreaseDeficit (int32_t deficit)
    {
        NS_LOG_FUNCTION (this << deficit);
        m_deficit -= deficit;
    }

    void
    DrrFlow::SetStatus (FlowStatus status)
    {
        m_status = status;
    }   

    DrrFlow::FlowStatus
    DrrFlow::GetStatus (void) const
    {
        return m_status;
    }

    NS_OBJECT_ENSURE_REGISTERED (DrrQueueDisc);

    TypeId DrrQueueDisc::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::DrrQueueDisc")
            .SetParent<QueueDisc> ()
            .SetGroupName ("TrafficControl")
            .AddConstructor<DrrQueueDisc> ()
            .AddAttribute ("Quantum",
                           "The quantum value to use",
                           UintegerValue (0),
                           MakeUintegerAccessor (&DrrQueueDisc::m_quantum),
                           MakeUintegerChecker<uint32_t> ())
        ;
        return tid;
    }

    //DrrQueueDisc constructor/destructor, SetQuantum, GetQuantum taken from FqCoDel queue code
    DrrQueueDisc::DrrQueueDisc ()
        : m_quantum (0)
    {
    }

    DrrQueueDisc::~DrrQueueDisc ()
    {
    }

    void
    DrrQueueDisc::SetQuantum (uint32_t quantum)
    {
        m_quantum = quantum;
    }

    uint32_t
    DrrQueueDisc::GetQuantum (void) const
    {
        return m_quantum;
    }

    bool
    DrrQueueDisc::CheckConfig (void)
    {
        return true;
    }

    bool 
    DrrQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
    {
        // Hash the IP 5-tuple to as identifier for the flow
        uint32_t index = item->Hash ();
        NS_LOG_DEBUG ("Hash output " << index << " for packet item " << item);

        //check if index is in m_flowIndicies. If not, add to m_flowIndicies and create flow.
        //following code adapted from FqCoDel code
        Ptr<DrrFlow> flow;
        if (m_flowsIndices.find (index) == m_flowsIndices.end ())
        {
            NS_LOG_DEBUG ("Creating a new flow queue with index " << index);
            flow = m_flowFactory.Create<DrrFlow> ();
            Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc> ();
            qd->Initialize ();
            flow->SetQueueDisc (qd);
            AddQueueDiscClass (flow);

            m_flowsIndices[index] = GetNQueueDiscClasses () - 1;
        }
        else
        {
            flow = StaticCast<DrrFlow> (GetQueueDiscClass (m_flowsIndices[index]));
        }

        if (flow->GetStatus () == DrrFlow::INACTIVE)
        {
            flow->SetStatus (DrrFlow::ACTIVE);
            flow->SetDeficit (m_quantum);
            m_activeList.push_back (flow);
        }

        //enqueue the flow
        flow->GetQueueDisc ()->Enqueue (item);

        //check if we need to drop packets.
        //FreeBuffer() using buffer stealing?? Currently have a DrrDrop that drops one packet

        return true;
    }

    Ptr<QueueDiscItem>
    DrrQueueDisc::DoDequeue (void)
    {
        //follow pseudocode from figure 4 of paper

        Ptr<DrrFlow> flow;
        Ptr<QueueDiscItem> item;

        do {
            if(!m_activeList.empty())
            {
                //remove head of active list
                flow = m_activeList.front();
                m_activeList.pop_front();

                //increment deficit
                flow->IncreaseDeficit(m_quantum);

                while(flow->GetDeficit () > 0 && flow->GetQueueDisc ()->GetNPackets () > 0)
                {
                    Ptr<const QueueDiscItem> head = flow->GetQueueDisc ()->Peek ();
                    uint32_t headSize = head->GetSize ();
                    NS_LOG_DEBUG("Head packet size " << headSize << " with deficit " << flow->GetDeficit ());
                    if (headSize <= flow->GetDeficit ())
                    {
                        flow->DecreaseDeficit (headSize);
                        Ptr<QueueDiscItem> item = flow->GetQueueDisc ()->Dequeue ();
                        
                        if(flow->GetQueueDisc()->GetNPackets() == 0)
                        {
                            //if the queue is empty, set deficit to zero, status to inactive
                            flow->SetDeficit(0);
                            flow->SetStatus(DrrFlow::INACTIVE);
                        }
                        else
                        {
                            //add flow back to front of the active list since the flow has packets left
                            m_activeList.push_front(flow);
                        }
                        return item;
                    } 
                    else
                    {
                        break;
                    }
                }

                if(flow->GetQueueDisc()->GetNPackets() == 0)
                {
                    //if the queue is empty, set deficit to zero, status to inactive
                    flow->SetDeficit(0);
                    flow->SetStatus(DrrFlow::INACTIVE);
                }
                else
                {
                    //add flow to end of the active list
                    m_activeList.push_back(flow);
                }
            }
            else
            {
                NS_LOG_DEBUG("No active flows to dequeue, returning null");
                return 0;
            }
        }
        while (true);
    }

    void
    DrrQueueDisc::InitializeParams (void)
    {
        m_flowFactory.SetTypeId ("ns3::DrrFlow");

        // Use FIFO queue for each flow (simplest logic)
        m_queueDiscFactory.SetTypeId ("ns3::FifoQueueDisc");
    }

    uint32_t
    DrrQueueDisc::DrrDrop (void)
    {
        // Commented out to pass compilation. Will revisit if needed

        // uint32_t maxBacklog = 0, index = 0;
        // Ptr<QueueDisc> qd;

        // /* Queue is full! Find the fat flow and drop packet(s) from it */
        // for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
        // {
        //     qd = GetQueueDiscClass (i)->GetQueueDisc ();
        //     uint32_t bytes = qd->GetNBytes ();
        //     if (bytes > maxBacklog)
        //     {
        //         maxBacklog = bytes;
        //         index = i;
        //     }
        // }   

        //drop just one packet. FqCoDel does similar, but drops half of the packets.
        //is it correct to just drop one?

        // uint32_t len = 0, count = 0, threshold = maxBacklog >> 1;
        // qd = GetQueueDiscClass (index)->GetQueueDisc ();
        // Ptr<QueueDiscItem> item;
        // item = qd->GetInternalQueue (0)->Dequeue ();
        // DropAfterDequeue (item, OVERLIMIT_DROP);
        // return index;

        return 0;
    
    }

} //end namespace ns3