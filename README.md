## What is cheetah ?

Cheetah is a job queue server, it receive jobs from clients and distribute them to idle workers. It helds 2 small queues: `waiting_workers`
and `waiting_job`. Cheetah pops two queues untill one of them is empty to identify job-worker tuple.

Cheetah focus on performace to beat some popular job queue libraries: [bee-queue](https://github.com/bee-queue/bee-queue), [bull-queue](https://github.com/OptimalBits/bull), [kue-queue](https://github.com/Automattic/kue), [agenda](https://github.com/agenda/agenda)

## Building cheetah

```bash
cd cheetah/
mkdir build/
cd build
cmake ..
make
```

## Protocol specification
To communicate between client and server, cheetah defines 2 types of data: `string` and `integer`
- For `string` the first byte is `"+"`
- For `integer` the first byte is `"-"`
- All of them are terminated by `"\r\n"`

For example:
- `112233` is encoded to `":112233\r\n"`
- `"hello"` is encoded to `"+hello\r\n"`

A valid packet to send and receive must contain 4 fields: `opcode` (integer) | `queue_name` (string) | `job_id` (integer) | `job_data` (string).
There are 6 types of packets:
- be_a_worker (opcode = 0): client wants to be a worker
- be_a_job (opcode = 1): client send a job to server
- be_a_result (opcode = 2): worker reports result to server
- be_a_progress (opcode = 3): worker reports progress to server
- process_it (opcode = 100): server forces a worker to process a job
- done_it (opcode = 101): server reports a result to client (who sent that job)
- progress_it (opcode = 102): server reports progress to client (who send that job)

For example: a client to send a job with id = 10, queue = mul, data = '{"x":1,y:"2"}' to server

`:1\r\n+mul\r\n:10\r\n+{"x":1,"y":2}\r\n` (31 bytes)
