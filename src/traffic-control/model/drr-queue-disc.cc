
//implementation of drr queue for NS-3
//Ang Li and Erin Voss
//much of the code is adopted from the fq-codel-queue-disc that
//is already implemented in NS3

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/queue.h"
#include "drr-queue-disc.h"
#include "codel-queue-disc.h"
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
        m_status (INACTIVE),
        m_index (0)
    {
        NS_LOG_FUNCTION (this);
    }

    DrrFlow::~DrrFlow ()
    {
        NS_LOG_FUNCTION (this);
    } 

    //SetDeficit, GetDeficit, IncreaseDeficit, SetStatus, FlowStatus, GetIndex, SetIndex taken from fq-codel-queue
    void
    DrrFlow::SetDeficit (uint32_t deficit)
    {
        NS_LOG_FUNCTION (this << deficit);
        m_deficit = deficit;
    }

    int32_t
    DrrFlow::GetDeficit (void) const
    {
        NS_LOG_FUNCTION (this);
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
        NS_LOG_FUNCTION (this);
        m_status = status;
    }   

    DrrFlow::FlowStatus
    DrrFlow::GetStatus (void) const
    {
        NS_LOG_FUNCTION (this);
        return m_status;
    }

    //might not need index for DRR?
    void
    DrrFlow::SetIndex (uint32_t index)
    {
        NS_LOG_FUNCTION (this);
        m_index = index;
    }

    uint32_t
    DrrFlow::GetIndex (void) const
    {
        return m_index;
    }

    NS_OBJECT_ENSURE_REGISTERED (DrrQueueDisc);

    TypeId DrrQueueDisc::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::DrrQueueDisc")
            .SetParent<QueueDisc> ()
            .SetGroupName ("TrafficControl")
            .AddConstructor<DrrQueueDisc> ()
            //add other attributes?
            // .AddAttribute ("UseEcn",
            //        "True to use ECN (packets are marked instead of being dropped)",
            //        BooleanValue (true),
            //        MakeBooleanAccessor (&FqCoDelQueueDisc::m_useEcn),
            //        MakeBooleanChecker ())
    ;
    return tid;
    }

    //DrrQueueDisc, SetQuantum, GetQUantum taken from FqCoDel queue code
    DrrQueueDisc::DrrQueueDisc ()
        : m_quantum (0),
          m_flows (0)
    {
        NS_LOG_FUNCTION (this);
    }

    DrrQueueDisc::~DrrQueueDisc ()
    {
        NS_LOG_FUNCTION (this);
    }

    void
    DrrQueueDisc::SetQuantum (uint32_t quantum)
    {
        NS_LOG_FUNCTION (this << quantum);
        m_quantum = quantum;
    }

    uint32_t
    DrrQueueDisc::GetQuantum (void) const
    {
        return m_quantum;
    }

    void
    DrrQueueDisc::SetFlows (uint32_t flows)
    {
        NS_LOG_FUNCTION (this << flows);
        m_flows = flows;
    }

    uint32_t
    DrrQueueDisc::GetFlows (void) const
    {
        return m_flows;
    }

    //Enqueue
    bool 
    DrrQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
    {

        NS_LOG_FUNCTION (this << item);

        //Extract flow from item
        int32_t ret = Classify (item);
        uint32_t index;

        if (ret != PacketFilter::PF_NO_MATCH)
        {
            index = ret % m_flows;
        }
        else
        {
          NS_LOG_ERROR ("No filter has been able to classify this packet, drop it.");
          //DropBeforeEnqueue (item, UNCLASSIFIED_DROP);
          //return false;

          //What should we do here? Either drop or create a seperate flow. Could use
          //index = max number output queues (m_flows)

        }

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


    }

    Ptr<QueueDiscItem>
    DrrQueueDisc::DoDequeue (void)
    {
        //follow pseudocode from figure 4 of paper
        NS_LOG_FUNCTION (this);

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

                while(flow->GetDeficit > 0 && flow->GetQueueDisc ()->getCurrentSize (). getValue() > 0)
                {
                    Ptr<const QueueDiscItem> head = flow->GetQueueDisc ()->Peek ();
                    uint32_t headSize = head.GetSize ();
                    if (headSize <= flow->GetDeficit ()) {
                        flow->DecreaseDeficit (headSize);
                        Ptr<QueueDiscItem> item = flow->GetQueueDisc ()->Dequeue ();
                        return item;
                    } else {
                        break;
                    }
                }

                //if the queue is empty, set deficit to zero, status to inactive
                if(flow->GetQueueDisc()->GetNPackets() == 0)
                {
                    flow->SetDeficit(0);
                    flow->SetStatus(DrrFlow::INACTIVE);
                    NS_LOG_DEBUG("Flow is empty, setting deficit to 0 and status to inactive");
                }
                else
                {
                    //add flow to end of the active list
                    m_activeList.push_back(flow);
                    NS_LOG_DEBUG("Flow still has packets, inserting at end of active list");
                }
            } else {
                NS_LOG_DEBUG("No active flows to dequeue, returning null");
                return null;
            }
        }
        while (true);

    }

    void
    DrrQueueDisc::InitializeParams (void)
    {
        NS_LOG_FUNCTION (this);

        m_flowFactory.SetTypeId ("ns3::DrrFlow");

        // is there a simpler base queue disc we can use here?
        m_queueDiscFactory.SetTypeId ("ns3::CoDelQueueDisc");
    }

    uint32_t
    DrrQueueDisc::DrrDrop (void)
    {
        NS_LOG_FUNCTION (this);

        uint32_t maxBacklog = 0, index = 0;
        Ptr<QueueDisc> qd;

        /* Queue is full! Find the fat flow and drop packet(s) from it */
        for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
        {
            qd = GetQueueDiscClass (i)->GetQueueDisc ();
            uint32_t bytes = qd->GetNBytes ();
            if (bytes > maxBacklog)
            {
                maxBacklog = bytes;
                index = i;
            }
        }   

        //drop just one packet. FqCoDel does similar, but drops half of the packets.
        //is it correct to just drop one?

        uint32_t len = 0, count = 0, threshold = maxBacklog >> 1;
        qd = GetQueueDiscClass (index)->GetQueueDisc ();
        Ptr<QueueDiscItem> item;
        item = qd->GetInternalQueue (0)->Dequeue ();
        DropAfterDequeue (item, OVERLIMIT_DROP);
        return index;
    
    }

} //end namespace ns3