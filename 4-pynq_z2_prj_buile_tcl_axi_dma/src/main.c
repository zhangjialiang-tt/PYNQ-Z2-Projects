#include "dma_intr.h"
#include "sys_intr.h"

static  XScuGic Intc; //GIC中断控制器
static  XAxiDma AxiDma;

int Tries = 6;
int i;
int Index;
u8 *TxBufferPtr= (u8 *)TX_BUFFER_BASE;
u8 *RxBufferPtr= (u8 *)RX_BUFFER_BASE;
u8 Value;

int axi_dma_test()
{
    int Status;
    TxDone = 0;
    RxDone = 0;
    Error = 0;

    xil_printf("\r\n----DMA Test----\r\n");
    xil_printf("PKT_LEN=%d\r\n",MAX_PKT_LEN);

    for(i = 0; i < Tries; i ++)
    {
        Value = 0x55 + (i & 0xFF);
        for(Index = 0; Index < MAX_PKT_LEN; Index ++) {
                TxBufferPtr[Index] = Value;

                Value = (Value + 1) & 0xFF;
        }

        Xil_DCacheFlushRange((u32)TxBufferPtr, MAX_PKT_LEN);//数据刷新到DDR中

        Status = XAxiDma_SimpleTransfer(&AxiDma,(u32) RxBufferPtr,//配置接收通道
                    MAX_PKT_LEN, XAXIDMA_DEVICE_TO_DMA);
        if (Status != XST_SUCCESS) {return XST_FAILURE;}

        Status = XAxiDma_SimpleTransfer(&AxiDma,(u32) TxBufferPtr,//配置发送通道
                    MAX_PKT_LEN, XAXIDMA_DMA_TO_DEVICE);
        if (Status != XST_SUCCESS) {return XST_FAILURE;}

        while (!TxDone || !RxDone) { }//等待收发中断都到达

        TxDone = 0;
        RxDone = 0;
        if (Error) {return XST_FAILURE;}

        Xil_DCacheInvalidateRange((u32)RxBufferPtr, MAX_PKT_LEN);//刷新cache，观察DDR的最新数据
    }

    DMA_DisableIntrSystem(&Intc, TX_INTR_ID, RX_INTR_ID);//失能DMA中断
    xil_printf("--- Exiting Test --- \r\n");
    return XST_SUCCESS;
}

int main(void)
{
    DMA_Intr_Init(&AxiDma,0);//DMA初始化
    Init_Intr_System(&Intc); //系统初始化
    Setup_Intr_Exception(&Intc);//使能硬件中断
    DMA_Setup_Intr_System(&Intc,&AxiDma,TX_INTR_ID,RX_INTR_ID);//注册DMA收发中断
    DMA_Intr_Enable(&Intc,&AxiDma);//使能系统中断

    axi_dma_test();
}