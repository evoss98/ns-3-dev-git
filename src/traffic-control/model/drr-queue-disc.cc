
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
    TypeId FqCoDelFlow::GetTypeId (void)
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
        : m_quantum (0)
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

    //Enqueue
    bool 
    DrrQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
    {
        //code for enqueue
    }

    Ptr<QueueDiscItem>
    DrrQueueDisc::DoDequeue (void)
    {
        //code for dequeue
    }

    //check config addapted from FqCoDelQueue code

    bool
    FqCoDelQueueDisc::CheckConfig (void)
    {
        NS_LOG_FUNCTION (this);
        if (GetNQueueDiscClasses () > 0)
        {
            NS_LOG_ERROR ("FqCoDelQueueDisc cannot have classes");
        return false;
        }

        if (GetNInternalQueues () > 0)
        {
            NS_LOG_ERROR ("FqCoDelQueueDisc cannot have internal queues");
            return false;
        }
    
        //more checks in FqCoDel, not sure if they are needed?
        
        return true;
    }

    void
    DrrQueueDisc::InitializeParams (void)
    {
        NS_LOG_FUNCTION (this);

        m_flowFactory.SetTypeId ("ns3::DrrFlow");

        //from FqCoDelCode
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