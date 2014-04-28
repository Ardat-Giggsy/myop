/*
 * elevator myop
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>

static const int read_weight = 1;
static const int write_weight = 1;

struct myop_data {
	struct list_head read_queue;
	struct list_head write_queue;
	int dir;
	int weight;
};

static int myop_dispatch(struct request_queue *q, int force)
{
	struct myop_data *nd = q->elevator->elevator_data;
	const int data_dir = nd -> dir;

	if(data_dir==READ){
		if(!list_empty(&nd->read_queue)){
			struct request *rq;
			rq = list_entry(nd->read_queue.next, struct request, queuelist);
			list_del_init(&rq->queuelist);
			elv_dispatch_sort(q, rq);
			nd -> weight = nd -> weight - 1;
			if(nd -> weight == 0)
			{
				nd->dir=WRITE;
				nd->weight = write_weight;
			}
			return 1;
		}else if(!list_empty(&nd->write_queue)){
			struct request *rq;
			rq = list_entry(nd->write_queue.next, struct request, queuelist);
			list_del_init(&rq->queuelist);
			elv_dispatch_sort(q, rq);
			return 1;
		}
	}else{
		if(!list_empty(&nd->write_queue)){
			struct request *rq;
			rq = list_entry(nd->write_queue.next, struct request, queuelist);
			list_del_init(&rq->queuelist);
			elv_dispatch_sort(q, rq);
			nd -> weight = nd -> weight - 1;
			if(nd -> weight == 0)
			{
				nd->dir = READ;
				nd->weight = read_weight;
			}
			return 1;
		}else if(!list_empty(&nd->read_queue)){
			struct request *rq;
			rq = list_entry(nd->read_queue.next, struct request, queuelist);
			list_del_init(&rq->queuelist);
			elv_dispatch_sort(q, rq);
			return 1;
		}
	}
	return 0;
}

static void myop_add_request(struct request_queue *q, struct request *rq)
{
	struct myop_data *nd = q->elevator->elevator_data;
	const int data_dir = rq_data_dir(rq);
	
	if(data_dir==READ){
		list_add_tail(&rq->queuelist, &nd->read_queue);
	}else{
		list_add_tail(&rq->queuelist, &nd->write_queue);
	}
}

static int myop_queue_empty(struct request_queue *q)
{
	struct myop_data *nd = q->elevator->elevator_data;

	return (list_empty(&nd->read_queue) || list_empty(&nd->write_queue));
}

static void *myop_init_queue(struct request_queue *q)
{
	struct myop_data *nd;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	INIT_LIST_HEAD(&nd->read_queue);
	INIT_LIST_HEAD(&nd->write_queue);
	nd->dir = WRITE;
	nd->weight = write_weight; 
	return nd;
}

static void myop_exit_queue(struct elevator_queue *e)
{
	struct myop_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->read_queue));
	BUG_ON(!list_empty(&nd->write_queue));
	kfree(nd);
}

static struct elevator_type elevator_myop = {
	.ops = {
		.elevator_dispatch_fn		= myop_dispatch,
		.elevator_add_req_fn		= myop_add_request,
		.elevator_queue_empty_fn	= myop_queue_empty,
		.elevator_init_fn		= myop_init_queue,
		.elevator_exit_fn		= myop_exit_queue,
	},
	.elevator_name = "myop",
	.elevator_owner = THIS_MODULE,
};

static int __init myop_init(void)
{
	elv_register(&elevator_myop);

	return 0;
}

static void __exit myop_exit(void)
{
	elv_unregister(&elevator_myop);
}

module_init(myop_init);
module_exit(myop_exit);


MODULE_AUTHOR("Rui Liu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A two-queue I/O Scheduler");
